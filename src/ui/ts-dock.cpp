#include "ts-dock.hpp"

#ifdef TOPSCREEN_HAVE_QT

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

namespace topscreen {
namespace {

/**
 * Dock do TopScreen.
 *
 * Fase inicial (T1.6 - parcial): controles de preset/toggle e status de input.
 * T1.8 liga estes controles ao filtro TopScreen selecionado.
 */
class TopScreenDock : public QWidget {
public:
	explicit TopScreenDock(QWidget *parent = nullptr) : QWidget(parent)
	{
		auto *root = new QVBoxLayout(this);

		auto *info = new QLabel(obs_module_text("Dock.Info"), this);
		info->setWordWrap(true);
		root->addWidget(info);

		auto *enabled = new QCheckBox(obs_module_text("Dock.EffectsEnabled"), this);
		enabled->setChecked(true);
		enabled->setEnabled(false); // TODO(T1.8): ligar ao filtro selecionado
		root->addWidget(enabled);

		auto *presetLabel = new QLabel(obs_module_text("Dock.Preset"), this);
		root->addWidget(presetLabel);

		auto *preset = new QComboBox(this);
		preset->addItems(QStringList() << "demo" << "tutorial" << "apresentacao");
		preset->setEnabled(false); // TODO(T1.8): aplicar preset ao vivo
		root->addWidget(preset);

		auto *status = new QLabel(QStringLiteral("%1: -").arg(obs_module_text("Dock.Input")), this);
		root->addWidget(status);

		root->addStretch(1);
	}
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
