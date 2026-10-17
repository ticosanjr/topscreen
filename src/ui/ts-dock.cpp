#include "ts-dock.hpp"

#ifdef TOPSCREEN_HAVE_QT

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "ts-registry.hpp"

namespace topscreen {
namespace {

/**
 * Dock do TopScreen.
 *
 * Operacoes do usuario sao enfileiradas no FilterControl (registro), que as
 * aplica na thread de video. O estado e atualizado por um QTimer.
 */
class TopScreenDock : public QWidget {
public:
	explicit TopScreenDock(QWidget *parent = nullptr) : QWidget(parent)
	{
		auto *root = new QVBoxLayout(this);

		info_ = new QLabel(obs_module_text("Dock.Info"), this);
		info_->setWordWrap(true);
		root->addWidget(info_);

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
		profile_->addItem(obs_module_text("Profile.A"));
		profile_->addItem(obs_module_text("Profile.B"));
		profile_->addItem(obs_module_text("Profile.C"));
		profile_->addItem(obs_module_text("Profile.D"));
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

		status_ = new QLabel(this);
		root->addWidget(status_);

		auto *refresh = new QPushButton(obs_module_text("Dock.Refresh"), this);
		QObject::connect(refresh, &QPushButton::clicked, this, [this]() { refresh_ui(); });
		root->addWidget(refresh);

		root->addStretch(1);

		timer_ = new QTimer(this);
		QObject::connect(timer_, &QTimer::timeout, this, [this]() { refresh_ui(); });
		timer_->start(500);

		refresh_ui();
	}

private:
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
		effects_->setEnabled(has);
		preset_->setEnabled(has);
		profile_->setEnabled(has);

		if (!has) {
			rebuild_presets(nullptr);
		} else {
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
	QCheckBox *effects_ = nullptr;
	QComboBox *preset_ = nullptr;
	QComboBox *profile_ = nullptr;
	QLabel *status_ = nullptr;
	QTimer *timer_ = nullptr;
	bool syncing_ = false;
};

TopScreenDock *g_dock = nullptr;

} // namespace

void ts_dock_register()
{
	g_dock = new TopScreenDock();
	obs_frontend_add_dock_by_id("topscreen_dock", obs_module_text("Dock.Title"), g_dock);
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
