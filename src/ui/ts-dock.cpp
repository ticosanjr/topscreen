#include "ts-dock.hpp"

#ifdef TOPSCREEN_HAVE_QT

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include <string>
#include <vector>

#include "audio/ts-click-source.hpp"
#include "webcam/ts-webcam-source.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "core/profiles_io.hpp"
#include "ts-registry.hpp"
#include "ts-scene-profiles.hpp"
#include "ts-version.hpp"

namespace topscreen {
namespace {

/**
 * TopScreen dock.
 *
 * User operations are queued in the FilterControl (registry), which applies
 * them on the video thread. The state is updated by a QTimer.
 */
class TopScreenDock : public QWidget {
public:
	explicit TopScreenDock(QWidget *parent = nullptr) : QWidget(parent)
	{
		auto *root = new QVBoxLayout(this);

		info_ = new QLabel(obs_module_text("Dock.Info"), this);
		info_->setWordWrap(true);
		root->addWidget(info_);

		root->addWidget(new QLabel(obs_module_text("Dock.Target"), this));
		target_ = new QComboBox(this);
		QObject::connect(target_, &QComboBox::currentIndexChanged, this, [this](int index) {
			if (syncing_ || index < 0)
				return;
			auto *control = static_cast<FilterControl *>(
				target_->itemData(index).value<void *>());
			if (!registry_select(control)) {
				refresh_ui();
				return;
			}
			refresh_ui();
		});
		root->addWidget(target_);

		effects_ = new QCheckBox(obs_module_text("Dock.EffectsEnabled"), this);
		QObject::connect(effects_, &QCheckBox::stateChanged, this, [this](int state) {
			if (syncing_)
				return;
			if (FilterControl *c = registry_first())
				c->set_effects_enabled(state != 0);
		});
		root->addWidget(effects_);

		root->addWidget(new QLabel(obs_module_text("Dock.Preset"), this));

		preset_ = new QComboBox(this);
		QObject::connect(preset_, &QComboBox::currentIndexChanged, this, [this](int index) {
			if (syncing_ || index < 0)
				return;
			if (FilterControl *c = registry_first())
				c->apply_preset(index);
		});
		root->addWidget(preset_);

		root->addWidget(new QLabel(obs_module_text("Profile.Current"), this));
		profile_ = new QComboBox(this);
		for (int i = 0; i < core::keys::kProfileCount; ++i)
			profile_->addItem(core::profile_label(i));
		QObject::connect(profile_, &QComboBox::currentIndexChanged, this, [this](int index) {
			if (syncing_ || index < 0)
				return;
			if (FilterControl *c = registry_first())
				c->select_profile(index);
		});
		root->addWidget(profile_);

		auto *saveProfile = new QPushButton(obs_module_text("Dock.SaveProfile"), this);
		QObject::connect(saveProfile, &QPushButton::clicked, this, [this]() {
			if (FilterControl *c = registry_first())
				c->save_profile();
		});
		root->addWidget(saveProfile);

		auto *stream_row = new QHBoxLayout();
		add_audio_btn_ = new QPushButton(obs_module_text("Dock.AddClickSound"), this);
		assign_scene_btn_ = new QPushButton(obs_module_text("Dock.AssignSceneProfile"), this);
		add_webcam_btn_ = new QPushButton(obs_module_text("Dock.AddWebcam"), this);
		QObject::connect(add_audio_btn_, &QPushButton::clicked, this, [this]() {
			if (ts_click_audio_add_to_current_scene())
				io_status_->setText(obs_module_text("Dock.ClickSoundAdded"));
			else
				io_status_->setText(obs_module_text("Dock.ClickSoundFailed"));
		});
		QObject::connect(assign_scene_btn_, &QPushButton::clicked, this, [this]() {
			FilterControl *c = registry_first();
			obs_source_t *scene = obs_frontend_get_current_scene();
			const char *name = scene ? obs_source_get_name(scene) : nullptr;
			const bool ok = c && name && scene_profiles::assign(name, c->current_profile());
			if (scene)
				obs_source_release(scene);
			if (ok)
				io_status_->setText(obs_module_text("Dock.SceneProfileAssigned"));
			else
				io_status_->setText(obs_module_text("Dock.SceneProfileFailed"));
		});
		QObject::connect(add_webcam_btn_, &QPushButton::clicked, this, [this]() {
			if (webcam_source::add_bubble_source())
				io_status_->setText(obs_module_text("Dock.WebcamAdded"));
			else
				io_status_->setText(obs_module_text("Dock.WebcamFailed"));
		});
		stream_row->addWidget(add_audio_btn_);
		stream_row->addWidget(assign_scene_btn_);
		stream_row->addWidget(add_webcam_btn_);
		root->addLayout(stream_row);

		auto *io_row = new QHBoxLayout();
		export_btn_ = new QPushButton(obs_module_text("Dock.ExportProfiles"), this);
		import_btn_ = new QPushButton(obs_module_text("Dock.ImportProfiles"), this);
		QObject::connect(export_btn_, &QPushButton::clicked, this, [this]() {
			export_profiles();
		});
		QObject::connect(import_btn_, &QPushButton::clicked, this, [this]() {
			import_profiles();
		});
		io_row->addWidget(export_btn_);
		io_row->addWidget(import_btn_);
		root->addLayout(io_row);

		io_status_ = new QLabel(this);
		io_status_->setWordWrap(true);
		root->addWidget(io_status_);

		status_ = new QLabel(this);
		root->addWidget(status_);

		scene_status_ = new QLabel(this);
		scene_status_->setWordWrap(true);
		root->addWidget(scene_status_);

		auto *refresh = new QPushButton(obs_module_text("Dock.Refresh"), this);
		QObject::connect(refresh, &QPushButton::clicked, this, [this]() { refresh_ui(); });
		root->addWidget(refresh);

		auto *version = new QLabel(QString("TopScreen ") + QString::fromUtf8(kVersion), this);
		root->addWidget(version);

		root->addStretch(1);

		timer_ = new QTimer(this);
		QObject::connect(timer_, &QTimer::timeout, this, [this]() { refresh_ui(); });
		timer_->start(500);

		refresh_ui();
	}

private:
	void export_profiles()
	{
		FilterControl *c = registry_first();
		if (!c)
			return;

		QString path = QFileDialog::getSaveFileName(
			this, obs_module_text("Dock.ExportProfiles"),
			QStringLiteral("topscreen-profiles.json"),
			QStringLiteral("TopScreen profiles (*.json);;All files (*)"));
		if (path.isEmpty())
			return;
		if (!path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
			path += QStringLiteral(".json");

		c->export_profiles(path.toStdString());
		io_status_->setText(obs_module_text("Dock.ExportStarted"));
	}

	void import_profiles()
	{
		FilterControl *c = registry_first();
		if (!c)
			return;

		const QString path = QFileDialog::getOpenFileName(
			this, obs_module_text("Dock.ImportProfiles"), QString(),
			QStringLiteral("TopScreen profiles (*.json);;All files (*)"));
		if (path.isEmpty())
			return;

		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::warning(this, QStringLiteral("TopScreen"),
					     obs_module_text("Dock.ImportUnreadable"));
			return;
		}
		if (file.size() > static_cast<qint64>(core::kMaxProfileBundleBytes)) {
			QMessageBox::warning(this, QStringLiteral("TopScreen"),
					     obs_module_text("Dock.ImportTooLarge"));
			return;
		}
		const QByteArray bytes = file.readAll();
		const std::string text(bytes.constData(), static_cast<size_t>(bytes.size()));

		core::ProfileBundleInfo info;
		std::string error;
		if (!core::profile_bundle_validate(text, &info, &error)) {
			QMessageBox::warning(this, QStringLiteral("TopScreen"),
					     QString(obs_module_text("Dock.ImportInvalid")) + "\n\n" +
						     QString::fromStdString(error));
			return;
		}

		const auto answer = QMessageBox::question(
			this, QStringLiteral("TopScreen"), obs_module_text("Dock.ImportConfirm"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (answer != QMessageBox::Yes)
			return;

		c->import_profiles(text);
		io_status_->setText(obs_module_text("Dock.ImportStarted"));
	}

	void rebuild_presets(FilterControl *c)
	{
		preset_->clear();
		if (c) {
			for (int i = 0; i < c->preset_count(); ++i)
				preset_->addItem(QString::fromStdString(c->preset_name(i)));
		}
	}

	void refresh_ui()
	{
		FilterControl *c = registry_first();
		const bool has = (c != nullptr);

		syncing_ = true;

		/* Rebuild the target selector only when the registry changed:
		 * clearing it every 500 ms would steal focus and reset the popup.
		 * The revision also avoids allocating a snapshot on every tick. */
		const std::uint64_t revision = registry_revision();
		if (revision != targets_revision_) {
			const std::vector<FilterEntry> entries = registry_list();
			target_->clear();
			QSet<QString> used;
			for (size_t i = 0; i < entries.size(); ++i) {
				QString label = QString::fromStdString(entries[i].name).trimmed();
				if (label.isEmpty())
					label = QStringLiteral("TopScreen #%1").arg(static_cast<int>(i) + 1);
				QString unique = label;
				int n = 2;
				while (used.contains(unique))
					unique = QStringLiteral("%1 (%2)").arg(label).arg(n++);
				used.insert(unique);
				target_->addItem(unique, QVariant::fromValue<void *>(entries[i].control));
			}
			targets_ = entries;
			targets_revision_ = revision;
		}
		target_->setEnabled(has);

		effects_->setEnabled(has);
		preset_->setEnabled(has);
		profile_->setEnabled(has);
		export_btn_->setEnabled(has);
		import_btn_->setEnabled(has);
		add_audio_btn_->setEnabled(has);
		assign_scene_btn_->setEnabled(has);
		add_webcam_btn_->setEnabled(has);

		if (!has) {
			rebuild_presets(nullptr);
		} else {
			int selected_target = -1;
			for (size_t i = 0; i < targets_.size(); ++i) {
				if (targets_[i].control == c) {
					selected_target = static_cast<int>(i);
					break;
				}
			}
			if (selected_target >= 0 && target_->currentIndex() != selected_target)
				target_->setCurrentIndex(selected_target);

			if (preset_->count() != c->preset_count())
				rebuild_presets(c);

			effects_->setChecked(c->effects_enabled());
			const int cur = c->current_preset();
			if (cur >= 0 && cur < preset_->count())
				preset_->setCurrentIndex(cur);

			const int pc = c->current_profile();
			if (pc >= 0 && pc < profile_->count())
				profile_->setCurrentIndex(pc);
		}
		syncing_ = false;

		/* A message from the previous filter would be misleading. */
		if (c != last_target_) {
			last_target_ = c;
			last_io_ = has ? c->profile_io_result() : 0;
			io_status_->clear();
		}

		if (!has) {
			io_status_->clear();
			last_io_ = 0;
		} else {
			const int io = c->profile_io_result();
			if (io != last_io_) {
				last_io_ = io;
				switch (io) {
				case 1:
					io_status_->setText(obs_module_text("Dock.ExportDone"));
					break;
				case 2:
					io_status_->setText(obs_module_text("Dock.ExportFailed"));
					break;
				case 3:
					io_status_->setText(obs_module_text("Dock.ImportDone"));
					break;
				case 4:
					io_status_->setText(obs_module_text("Dock.ImportFailed"));
					break;
				default:
					/* 0 = a new request; keep the "started" message. */
					break;
				}
			}
		}

		{
			obs_source_t *scene = obs_frontend_get_current_scene();
			const char *raw_name = scene ? obs_source_get_name(scene) : nullptr;
			const std::string scene_name = raw_name ? raw_name : "";
			const int profile = scene_name.empty() ? -1 : scene_profiles::get(scene_name);
			if (scene)
				obs_source_release(scene);
			if (!has || scene_name.empty())
				scene_status_->clear();
			else if (profile >= 0)
				scene_status_->setText(
					QString(obs_module_text("Dock.SceneProfile")) + ": " +
					QString::fromUtf8(core::profile_label(profile)));
			else
				scene_status_->setText(obs_module_text("Dock.NoSceneProfile"));
		}

		if (has) {
			const int tier = c->input_tier();
			const char *tier_text = obs_module_text("Tier.None");
			if (tier == 1)
				tier_text = obs_module_text("Tier.Native");
			else if (tier == 2)
				tier_text = obs_module_text("Tier.Portal");
			else if (tier == 3)
				tier_text = obs_module_text("Tier.Evdev");

			status_->setText(QString(obs_module_text("Dock.Input")) + ": " + QString(tier_text));
		} else {
			status_->setText(QString(obs_module_text("Dock.NoFilter")));
		}
	}

	QLabel *info_ = nullptr;
	QComboBox *target_ = nullptr;
	QCheckBox *effects_ = nullptr;
	QComboBox *preset_ = nullptr;
	QComboBox *profile_ = nullptr;
	QPushButton *export_btn_ = nullptr;
	QPushButton *import_btn_ = nullptr;
	QPushButton *add_audio_btn_ = nullptr;
	QPushButton *assign_scene_btn_ = nullptr;
	QPushButton *add_webcam_btn_ = nullptr;
	QLabel *io_status_ = nullptr;
	QLabel *status_ = nullptr;
	QLabel *scene_status_ = nullptr;
	QTimer *timer_ = nullptr;
	std::vector<FilterEntry> targets_;
	std::uint64_t targets_revision_ = 0;
	bool syncing_ = false;
	FilterControl *last_target_ = nullptr;
	int last_io_ = 0;
};

TopScreenDock *g_dock = nullptr;

} // namespace

void ts_dock_register()
{
	g_dock = new TopScreenDock();
	static const std::string title =
		std::string(obs_module_text("Dock.Title")) + " " + kVersion;
	obs_frontend_add_dock_by_id("topscreen_dock", title.c_str(), g_dock);
}

void ts_dock_unregister()
{
	if (g_dock) {
		obs_frontend_remove_dock("topscreen_dock");
		g_dock = nullptr;
	}
}

} // namespace topscreen

#else

namespace topscreen {

void ts_dock_register() {}
void ts_dock_unregister() {}

} // namespace topscreen

#endif
