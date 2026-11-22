#include "ts-dock.hpp"

#ifdef TOPSCREEN_HAVE_QT

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QUrl>
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
		auto *outer = new QVBoxLayout(this);
		outer->setContentsMargins(0, 0, 0, 0);
		auto *scroll = new QScrollArea(this);
		scroll->setWidgetResizable(true);
		auto *content = new QWidget(scroll);
		auto *root = new QVBoxLayout(content);
		scroll->setWidget(content);
		outer->addWidget(scroll);

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

		/* ---- Live ---------------------------------------------------- */
		auto *live = new QGroupBox(obs_module_text("Dock.LiveGroup"), this);
		auto *live_layout = new QVBoxLayout(live);
		effects_ = new QCheckBox(obs_module_text("Dock.EffectsEnabled"), live);
		QObject::connect(effects_, &QCheckBox::toggled, this, [this](bool on) {
			if (syncing_)
				return;
			if (FilterControl *c = registry_first())
				c->set_effects_enabled(on);
		});
		live_layout->addWidget(effects_);
		keys_pause_ = new QCheckBox(obs_module_text("Dock.PauseKeys"), live);
		QObject::connect(keys_pause_, &QCheckBox::toggled, this, [this](bool on) {
			if (syncing_)
				return;
			if (FilterControl *c = registry_first()) {
				if (c->keys_paused() != on)
					c->toggle_keys_pause();
			}
		});
		live_layout->addWidget(keys_pause_);
		keys_status_ = new QLabel(live);
		live_layout->addWidget(keys_status_);
		root->addWidget(live);

		/* ---- Look ---------------------------------------------------- */
		auto *look = new QGroupBox(obs_module_text("Dock.LookGroup"), this);
		auto *look_layout = new QVBoxLayout(look);
		look_layout->addWidget(new QLabel(obs_module_text("Dock.Preset"), look));
		preset_ = new QComboBox(look);
		QObject::connect(preset_, &QComboBox::currentIndexChanged, this, [this](int index) {
			if (syncing_ || index < 0)
				return;
			if (FilterControl *c = registry_first())
				c->apply_preset(index);
		});
		look_layout->addWidget(preset_);
		auto *look_row = new QHBoxLayout();
		save_look_btn_ = new QPushButton(obs_module_text("Dock.SaveLook"), look);
		delete_look_btn_ = new QPushButton(obs_module_text("Dock.DeleteLook"), look);
		open_looks_btn_ = new QPushButton(obs_module_text("Dock.OpenLooksFolder"), look);
		QObject::connect(save_look_btn_, &QPushButton::clicked, this, [this]() {
			bool ok = false;
			const QString name = QInputDialog::getText(
				this, QStringLiteral("TopScreen"),
				obs_module_text("Dock.LookName"), QLineEdit::Normal,
				QString(), &ok);
			if (ok && !name.trimmed().isEmpty()) {
				if (FilterControl *c = registry_first())
					c->save_look(name.trimmed().toStdString());
			}
		});
		QObject::connect(delete_look_btn_, &QPushButton::clicked, this, [this]() {
			FilterControl *c = registry_first();
			if (!c)
				return;
			const int index = c->current_preset();
			if (index < 0 || !c->is_user_look(index))
				return;
			const auto answer = QMessageBox::question(
				this, QStringLiteral("TopScreen"),
				obs_module_text("Dock.DeleteLookConfirm"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (answer == QMessageBox::Yes)
				c->delete_look(index);
		});
		QObject::connect(open_looks_btn_, &QPushButton::clicked, this, [this]() {
			if (FilterControl *c = registry_first()) {
				const std::string dir = c->look_dir();
				if (!dir.empty())
					QDesktopServices::openUrl(
						QUrl::fromLocalFile(QString::fromStdString(dir)));
			}
		});
		look_row->addWidget(save_look_btn_);
		look_row->addWidget(delete_look_btn_);
		look_row->addWidget(open_looks_btn_);
		look_layout->addLayout(look_row);
		look_layout->addWidget(new QLabel(obs_module_text("Profile.Current"), look));
		profile_ = new QComboBox(look);
		for (int i = 0; i < core::keys::kProfileCount; ++i)
			profile_->addItem(core::profile_label(i));
		QObject::connect(profile_, &QComboBox::currentIndexChanged, this, [this](int index) {
			if (syncing_ || index < 0)
				return;
			if (FilterControl *c = registry_first())
				c->select_profile(index);
		});
		look_layout->addWidget(profile_);

		auto *name_row = new QHBoxLayout();
		profile_name_ = new QLineEdit(look);
		profile_name_->setPlaceholderText(obs_module_text("Dock.ProfileNameHint"));
		rename_btn_ = new QPushButton(obs_module_text("Dock.Rename"), look);
		auto rename_current = [this]() {
			if (syncing_)
				return;
			if (FilterControl *c = registry_first())
				c->rename_profile(c->current_profile(),
						  profile_name_->text().trimmed().toStdString());
		};
		QObject::connect(rename_btn_, &QPushButton::clicked, this, rename_current);
		QObject::connect(profile_name_, &QLineEdit::returnPressed, this, rename_current);
		name_row->addWidget(profile_name_);
		name_row->addWidget(rename_btn_);
		look_layout->addLayout(name_row);

		auto *dup_row = new QHBoxLayout();
		duplicate_ = new QComboBox(look);
		duplicate_btn_ = new QPushButton(obs_module_text("Dock.DuplicateButton"), look);
		QObject::connect(duplicate_btn_, &QPushButton::clicked, this, [this]() {
			if (syncing_)
				return;
			if (FilterControl *c = registry_first()) {
				const int target = duplicate_->currentData().toInt();
				if (target >= 0)
					c->duplicate_profile(target);
			}
		});
		dup_row->addWidget(duplicate_);
		dup_row->addWidget(duplicate_btn_);
		look_layout->addLayout(dup_row);

		revert_btn_ = new QPushButton(obs_module_text("Dock.Revert"), look);
		QObject::connect(revert_btn_, &QPushButton::clicked, this, [this]() {
			if (FilterControl *c = registry_first())
				c->revert_profile();
		});
		look_layout->addWidget(revert_btn_);

		save_btn_ = new QPushButton(obs_module_text("Dock.SaveProfile"), look);
		QObject::connect(save_btn_, &QPushButton::clicked, this, [this]() {
			if (FilterControl *c = registry_first())
				c->save_profile();
		});
		look_layout->addWidget(save_btn_);
		root->addWidget(look);

		/* ---- Scene --------------------------------------------------- */
		auto *scene = new QGroupBox(obs_module_text("Dock.SceneGroup"), this);
		auto *scene_layout = new QVBoxLayout(scene);
		assign_scene_btn_ = new QPushButton(obs_module_text("Dock.AssignSceneProfile"), scene);
		QObject::connect(assign_scene_btn_, &QPushButton::clicked, this, [this]() {
			FilterControl *c = registry_first();
			obs_source_t *current = obs_frontend_get_current_scene();
			const bool ok = c && current &&
					scene_profiles::assign(current, c->current_profile());
			if (current)
				obs_source_release(current);
			if (ok)
				io_status_->setText(obs_module_text("Dock.SceneProfileAssigned"));
			else
				io_status_->setText(obs_module_text("Dock.SceneProfileFailed"));
		});
		scene_layout->addWidget(assign_scene_btn_);
		scene_status_ = new QLabel(scene);
		scene_status_->setWordWrap(true);
		scene_layout->addWidget(scene_status_);
		root->addWidget(scene);

		/* ---- Sources ------------------------------------------------ */
		auto *sources = new QGroupBox(obs_module_text("Dock.SourcesGroup"), this);
		auto *sources_layout = new QVBoxLayout(sources);
		add_audio_btn_ = new QPushButton(obs_module_text("Dock.AddClickSound"), sources);
		add_webcam_btn_ = new QPushButton(obs_module_text("Dock.AddWebcam"), sources);
		QObject::connect(add_audio_btn_, &QPushButton::clicked, this, [this]() {
			if (ts_click_audio_add_to_current_scene())
				io_status_->setText(obs_module_text("Dock.ClickSoundAdded"));
			else
				io_status_->setText(obs_module_text("Dock.ClickSoundFailed"));
		});
		QObject::connect(add_webcam_btn_, &QPushButton::clicked, this, [this]() {
			if (webcam_source::add_bubble_source())
				io_status_->setText(obs_module_text("Dock.WebcamAdded"));
			else
				io_status_->setText(obs_module_text("Dock.WebcamFailed"));
		});
		sources_layout->addWidget(add_audio_btn_);
		sources_layout->addWidget(add_webcam_btn_);
		root->addWidget(sources);

		/* ---- Backup ------------------------------------------------- */
		auto *backup = new QGroupBox(obs_module_text("Dock.BackupGroup"), this);
		auto *backup_layout = new QVBoxLayout(backup);
		auto *io_row = new QHBoxLayout();
		export_btn_ = new QPushButton(obs_module_text("Dock.ExportProfiles"), backup);
		import_btn_ = new QPushButton(obs_module_text("Dock.ImportProfiles"), backup);
		QObject::connect(export_btn_, &QPushButton::clicked, this, [this]() {
			export_profiles();
		});
		QObject::connect(import_btn_, &QPushButton::clicked, this, [this]() {
			import_profiles();
		});
		io_row->addWidget(export_btn_);
		io_row->addWidget(import_btn_);
		backup_layout->addLayout(io_row);
		reset_btn_ = new QPushButton(obs_module_text("Dock.Reset"), backup);
		QObject::connect(reset_btn_, &QPushButton::clicked, this, [this]() {
			if (!registry_first())
				return;
			const auto answer = QMessageBox::question(
				this, QStringLiteral("TopScreen"),
				obs_module_text("Dock.ResetConfirm"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (answer != QMessageBox::Yes)
				return;
			if (FilterControl *c = registry_first())
				c->reset_profiles();
			io_status_->setText(obs_module_text("Dock.ResetStarted"));
		});
		backup_layout->addWidget(reset_btn_);
		io_status_ = new QLabel(backup);
		io_status_->setWordWrap(true);
		backup_layout->addWidget(io_status_);
		root->addWidget(backup);

		/* ---- Diagnostics -------------------------------------------- */
		auto *diag = new QGroupBox(obs_module_text("Dock.DiagnosticsGroup"), this);
		auto *diag_layout = new QVBoxLayout(diag);
		source_status_ = new QLabel(diag);
		diag_layout->addWidget(source_status_);
		status_ = new QLabel(diag);
		status_->setWordWrap(true);
		diag_layout->addWidget(status_);
		monitor_status_ = new QLabel(diag);
		monitor_status_->setWordWrap(true);
		diag_layout->addWidget(monitor_status_);
		copy_diag_btn_ = new QPushButton(obs_module_text("Dock.CopyDiagnostics"), diag);
		QObject::connect(copy_diag_btn_, &QPushButton::clicked, this, [this]() {
			QGuiApplication::clipboard()->setText(
				status_->text() + "\n" + monitor_status_->text() + "\n" +
				keys_status_->text() + "\nTopScreen " + QString::fromUtf8(kVersion));
			io_status_->setText(obs_module_text("Dock.DiagnosticsCopied"));
		});
		diag_layout->addWidget(copy_diag_btn_);
		auto *manual_btn = new QPushButton(obs_module_text("Dock.OpenManual"), diag);
		QObject::connect(manual_btn, &QPushButton::clicked, this, []() {
			QDesktopServices::openUrl(
				QUrl(QStringLiteral("https://github.com/ticosanjr/topscreen/blob/main/docs/MANUAL.md")));
		});
		diag_layout->addWidget(manual_btn);
		auto *version = new QLabel(QString("TopScreen ") + QString::fromUtf8(kVersion), diag);
		diag_layout->addWidget(version);
		root->addWidget(diag);

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

		c->export_profiles(path.toStdString(), scene_profiles::export_json());
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

		if (!scene_profiles::import_from_bundle(text))
			blog(LOG_WARNING, "[TopScreen] profiles import: invalid scene profile map");
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
		keys_pause_->setEnabled(has);
		preset_->setEnabled(has);
		save_look_btn_->setEnabled(has);
		open_looks_btn_->setEnabled(has);
		profile_->setEnabled(has);
		profile_name_->setEnabled(has);
		rename_btn_->setEnabled(has);
		duplicate_->setEnabled(has);
		duplicate_btn_->setEnabled(has);
		revert_btn_->setEnabled(has);
		save_btn_->setEnabled(has);
		export_btn_->setEnabled(has);
		import_btn_->setEnabled(has);
		reset_btn_->setEnabled(has);
		add_audio_btn_->setEnabled(has);
		assign_scene_btn_->setEnabled(has);
		add_webcam_btn_->setEnabled(has);
		copy_diag_btn_->setEnabled(has);

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
			else
				preset_->setCurrentIndex(-1); /* no active preset */
			delete_look_btn_->setEnabled(cur >= 0 && c->is_user_look(cur));

			const int pc = c->current_profile();
			for (int i = 0; i < profile_->count(); ++i)
				profile_->setItemText(i, QString::fromStdString(c->profile_name(i)));
			if (pc >= 0 && pc < profile_->count())
				profile_->setCurrentIndex(pc);
			if (!profile_name_->hasFocus())
				profile_name_->setText(QString::fromStdString(c->profile_name(pc)));

			if (pc != duplicate_for_) {
				duplicate_for_ = pc;
				duplicate_->clear();
				for (int i = 0; i < c->profile_count(); ++i) {
					if (i == pc)
						continue;
					duplicate_->addItem(
						QString::fromStdString(c->profile_name(i)), i);
				}
			}
			for (int i = 0; i < duplicate_->count(); ++i) {
				const int slot = duplicate_->itemData(i).toInt();
				duplicate_->setItemText(
					i, QString::fromStdString(c->profile_name(slot)));
			}

			revert_btn_->setEnabled(c->can_revert_profile());
			keys_pause_->setEnabled(c->keys_enabled());
			keys_pause_->setChecked(c->keys_paused());
			if (!c->keys_enabled())
				keys_status_->setText(QString(obs_module_text("Dock.KeysOff")));
			else if (c->keys_paused())
				keys_status_->setText(QString(obs_module_text("Dock.KeysPaused")));
			else
				keys_status_->setText(QString(obs_module_text("Dock.KeysLive")));
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
			const int profile = scene ? scene_profiles::get(scene) : -1;
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

			QString input =
				QString(obs_module_text("Dock.Input")) + ": " + QString(tier_text);
			if (tier == 0)
				input += "\n" + QString(obs_module_text("Dock.InputUnavailable"));
#if defined(__linux__)
			if (c->keys_enabled() && tier != 3)
				input += "\n" + QString(obs_module_text("Dock.KeysNeedEvdev"));
#endif
			status_->setText(input);

			source_status_->setText(
				QString(obs_module_text("Dock.SourceType")) + ": " +
				QString(c->is_capture_source() ? obs_module_text("Dock.SourceCapture")
							       : obs_module_text("Dock.SourceFilter")));

			double origin_x = 0.0, origin_y = 0.0;
			c->monitor_origin(&origin_x, &origin_y);
			monitor_status_->setText(
				QString(obs_module_text("Dock.MonitorOrigin")) + ": " +
				QString::number(origin_x, 'f', 0) + ", " +
				QString::number(origin_y, 'f', 0));
		} else {
			status_->setText(QString(obs_module_text("Dock.NoFilter")));
			source_status_->clear();
			monitor_status_->clear();
			keys_status_->clear();
			profile_name_->clear();
			duplicate_->clear();
			duplicate_for_ = -1;
		}
	}

	QLabel *info_ = nullptr;
	QComboBox *target_ = nullptr;
	QCheckBox *effects_ = nullptr;
	QCheckBox *keys_pause_ = nullptr;
	QComboBox *preset_ = nullptr;
	QPushButton *save_look_btn_ = nullptr;
	QPushButton *delete_look_btn_ = nullptr;
	QPushButton *open_looks_btn_ = nullptr;
	QComboBox *profile_ = nullptr;
	QLineEdit *profile_name_ = nullptr;
	QPushButton *rename_btn_ = nullptr;
	QComboBox *duplicate_ = nullptr;
	QPushButton *duplicate_btn_ = nullptr;
	QPushButton *revert_btn_ = nullptr;
	QPushButton *save_btn_ = nullptr;
	QPushButton *export_btn_ = nullptr;
	QPushButton *import_btn_ = nullptr;
	QPushButton *reset_btn_ = nullptr;
	QPushButton *add_audio_btn_ = nullptr;
	QPushButton *assign_scene_btn_ = nullptr;
	QPushButton *add_webcam_btn_ = nullptr;
	QPushButton *copy_diag_btn_ = nullptr;
	QLabel *io_status_ = nullptr;
	QLabel *source_status_ = nullptr;
	QLabel *status_ = nullptr;
	QLabel *monitor_status_ = nullptr;
	QLabel *keys_status_ = nullptr;
	QLabel *scene_status_ = nullptr;
	QTimer *timer_ = nullptr;
	int duplicate_for_ = -1;
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
