#include "ts-properties.hpp"

#include "core/easing.hpp"
#include "core/keys.hpp"
#include "core/types.hpp"
#include "ts-wallpapers.hpp"

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen {

namespace {

/* Fontes embutidas (data/fonts). */
const char *const kFonts[][2] = {
	{"DejaVu Sans Bold", "fonts/DejaVuSans-Bold.ttf"},
	{"DejaVu Sans", "fonts/DejaVuSans.ttf"},
	{"DejaVu Sans Mono Bold", "fonts/DejaVuSansMono-Bold.ttf"},
};

/* Filtro do seletor de imagem (hardcoded, como no OBS: evita risco de escaping). */
const char *const kImageFilter =
	"Images (*.png *.jpg *.jpeg *.bmp *.tga *.gif *.webp *.PNG *.JPG *.JPEG *.BMP *.TGA *.GIF *.WEBP)"
	";;All files (*.*)";

/* Wallpaper e Imagem do PC sao mutuamente exclusivos. */
bool wallpaper_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)props;
	(void)p;
	const char *v = obs_data_get_string(settings, kBgImage);
	if (v && *v)
		obs_data_set_string(settings, kBgImageFile, "");
	return true;
}

bool bg_file_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)props;
	(void)p;
	const char *v = obs_data_get_string(settings, kBgImageFile);
	if (v && *v)
		obs_data_set_string(settings, kBgImage, "");
	return true;
}

void add_click_group(obs_properties_t *props, const char *group_key, const char *title,
		     const char *effect_key, const char *size_key, const char *color_key,
		     const char *duration_key, const char *easing_key)
{
	obs_properties_t *g = obs_properties_create();

	obs_property_t *effect = obs_properties_add_list(g, effect_key, obs_module_text("Click.Effect"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Ripple"),
				  static_cast<int>(ClickEffect::Ripple));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Orb"),
				  static_cast<int>(ClickEffect::Orb));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Ring"),
				  static_cast<int>(ClickEffect::Ring));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Force"),
				  static_cast<int>(ClickEffect::Force));

	obs_properties_add_float_slider(g, size_key, obs_module_text("Click.Size"), 0.1, 4.0, 0.05);
	obs_properties_add_color(g, color_key, obs_module_text("Click.Color"));
	obs_properties_add_float_slider(g, duration_key, obs_module_text("Click.DurationMs"), 100.0, 2000.0, 10.0);

	obs_property_t *easing = obs_properties_add_list(g, easing_key, obs_module_text("Click.Easing"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(easing, obs_module_text("Easing.Linear"), EASE_LINEAR);
	obs_property_list_add_int(easing, obs_module_text("Easing.InCubic"), EASE_IN_CUBIC);
	obs_property_list_add_int(easing, obs_module_text("Easing.OutCubic"), EASE_OUT_CUBIC);
	obs_property_list_add_int(easing, obs_module_text("Easing.InOutCubic"), EASE_IN_OUT_CUBIC);

	obs_properties_add_group(props, group_key, title, OBS_GROUP_NORMAL, g);
}

} // namespace

obs_properties_t *properties_create(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_bool(props, kEffectsEnabled, obs_module_text("Enabled"));

	/* ---------------- Fundo ---------------- */
	obs_properties_t *bg = obs_properties_create();
	obs_properties_add_bool(bg, kBgEnabled, obs_module_text("Background.Enabled"));

	obs_property_t *mode = obs_properties_add_list(bg, kBgMode, obs_module_text("Background.Mode"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Solid"), static_cast<int>(BgMode::Solid));
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Vertical"),
				  static_cast<int>(BgMode::Vertical));
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Diagonal"),
				  static_cast<int>(BgMode::Diagonal));

	/* Lista NAO-editavel: mostra o rotulo, salva o caminho. */
	obs_property_t *wp = obs_properties_add_list(bg, kBgImage, obs_module_text("Background.Image"),
						     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback(wp, wallpaper_modified);
	obs_property_list_add_string(wp, obs_module_text("Background.Image.None"), "");
	for (const auto &entry : kWallpapers)
		obs_property_list_add_string(wp, entry.name, entry.path);

	obs_property_t *fp = obs_properties_add_path(bg, kBgImageFile, obs_module_text("Background.ImageFile"),
						     OBS_PATH_FILE, kImageFilter, nullptr);
	obs_property_set_modified_callback(fp, bg_file_modified);
	obs_properties_add_float_slider(bg, kBgImageBlur, obs_module_text("Background.ImageBlur"), 0.0, 60.0, 1.0);
	obs_property_t *bt = obs_properties_add_list(bg, kBgImageBlurType, obs_module_text("Background.ImageBlurType"),
						     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(bt, obs_module_text("Blur.None"), 0);
	obs_property_list_add_int(bt, obs_module_text("Blur.Soft"), 1);
	obs_property_list_add_int(bt, obs_module_text("Blur.Gaussian"), 2);
	obs_property_list_add_int(bt, obs_module_text("Blur.Creamy"), 3);

	obs_properties_add_color(bg, kBgColorA, obs_module_text("Background.ColorA"));
	obs_properties_add_color(bg, kBgColorB, obs_module_text("Background.ColorB"));
	obs_properties_add_float_slider(bg, kBgPaddingX, obs_module_text("Background.PaddingX"), 0.0, 400.0, 1.0);
	obs_properties_add_float_slider(bg, kBgPaddingY, obs_module_text("Background.PaddingY"), 0.0, 400.0, 1.0);
	obs_properties_add_bool(bg, kBgKeepAspect, obs_module_text("Background.KeepAspect"));
	obs_properties_add_float_slider(bg, kBgCornerRadius, obs_module_text("Background.CornerRadius"), 0.0, 200.0,
					1.0);

	/* Sombra da janela (sub-grupo com nomes claros) */
	obs_properties_t *sh = obs_properties_create();
	obs_properties_add_bool(sh, kBgShadow, obs_module_text("Shadow.Enabled"));
	obs_properties_add_float_slider(sh, kBgShadowAlpha, obs_module_text("Shadow.Opacity"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(sh, kBgShadowBlur, obs_module_text("Shadow.Blur"), 0.0, 200.0, 1.0);
	obs_properties_add_float_slider(sh, kBgShadowOffset, obs_module_text("Shadow.Offset"), -100.0, 100.0, 1.0);
	obs_properties_add_group(bg, "grp_shadow", obs_module_text("Group.Shadow"), OBS_GROUP_NORMAL, sh);
	obs_properties_add_group(props, "grp_bg", obs_module_text("Group.Background"), OBS_GROUP_NORMAL, bg);

	/* ---------------- Cursor ---------------- */
	obs_properties_t *cur = obs_properties_create();
	obs_properties_add_bool(cur, kCursorEnabled, obs_module_text("Cursor.Enabled"));

	obs_property_t *style = obs_properties_add_list(cur, kCursorStyle, obs_module_text("Cursor.Style"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.MacOs"),
				  static_cast<int>(CursorStyle::MacOs));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Windows"),
				  static_cast<int>(CursorStyle::Windows));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Breeze"),
				  static_cast<int>(CursorStyle::Breeze));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Bibata"),
				  static_cast<int>(CursorStyle::Bibata));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Adwaita"),
				  static_cast<int>(CursorStyle::Adwaita));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.GoogleDot"),
				  static_cast<int>(CursorStyle::GoogleDot));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Pointer"),
				  static_cast<int>(CursorStyle::Pointer));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Dmz"),
				  static_cast<int>(CursorStyle::Dmz));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Yaru"),
				  static_cast<int>(CursorStyle::Yaru));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.XCursorPro"),
				  static_cast<int>(CursorStyle::XCursorPro));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.BibataIce"),
				  static_cast<int>(CursorStyle::BibataIce));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.BibataOriginal"),
				  static_cast<int>(CursorStyle::BibataOriginal));

	obs_properties_add_float_slider(cur, kCursorSize, obs_module_text("Cursor.Size"), 0.5, 4.0, 0.05);
	obs_properties_add_float_slider(cur, kCursorSmooth, obs_module_text("Cursor.Smooth"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(cur, kCursorSway, obs_module_text("Cursor.Sway"), 0.0, 1.0, 0.01);
	obs_properties_add_bool(cur, kCursorShadow, obs_module_text("Cursor.Shadow"));
	obs_properties_add_bool(cur, kCursorHideIdle, obs_module_text("Cursor.HideIdle"));
	obs_properties_add_float_slider(cur, kCursorIdleDelayMs, obs_module_text("Cursor.IdleDelayMs"), 0.0,
					5000.0, 50.0);
	obs_properties_add_bool(cur, kCursorLoop, obs_module_text("Cursor.Loop"));
	obs_properties_add_float_slider(cur, kCursorLoopMs, obs_module_text("Cursor.LoopMs"), 1000.0, 20000.0,
					100.0);
	obs_properties_add_group(props, "grp_cursor", obs_module_text("Group.Cursor"), OBS_GROUP_NORMAL, cur);

	/* ---------------- Cliques ---------------- */
	add_click_group(props, "grp_click_left", obs_module_text("Group.ClickLeft"), kClickLeftEffect,
			kClickLeftSize, kClickLeftColor, kClickLeftDurationMs, kClickLeftEasing);
	add_click_group(props, "grp_click_right", obs_module_text("Group.ClickRight"), kClickRightEffect,
			kClickRightSize, kClickRightColor, kClickRightDurationMs, kClickRightEasing);

	/* ---------------- Som de clique ---------------- */
	obs_properties_t *snd = obs_properties_create();
	obs_properties_add_bool(snd, kClickSoundEnabled, obs_module_text("ClickSound.Enabled"));
	obs_properties_add_float_slider(snd, kClickSoundVolume, obs_module_text("ClickSound.Volume"), 0.0, 1.0,
					0.01);
	obs_properties_add_bool(snd, kClickSoundLeft, obs_module_text("ClickSound.Left"));
	obs_properties_add_bool(snd, kClickSoundRight, obs_module_text("ClickSound.Right"));
	obs_properties_add_group(props, "grp_sound", obs_module_text("Group.ClickSound"), OBS_GROUP_NORMAL, snd);

	/* ---------------- Zoom ---------------- */
	obs_properties_t *zoom = obs_properties_create();
	obs_property_t *zmode = obs_properties_add_list(zoom, kZoomMode, obs_module_text("Zoom.Mode"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Off"), static_cast<int>(ZoomMode::Off));
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Auto"), static_cast<int>(ZoomMode::Auto));
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Manual"), static_cast<int>(ZoomMode::Manual));

	obs_properties_add_float_slider(zoom, kZoomAmount, obs_module_text("Zoom.Amount"), 1.0, 4.0, 0.05);
	obs_properties_add_float_slider(zoom, kZoomDurationMs, obs_module_text("Zoom.DurationMs"), 100.0, 2000.0,
					10.0);
	obs_properties_add_float_slider(zoom, kZoomIdleTimeoutMs, obs_module_text("Zoom.IdleTimeoutMs"), 200.0,
					10000.0, 50.0);
	obs_properties_add_float_slider(zoom, kZoomPanAmount, obs_module_text("Zoom.PanAmount"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(zoom, kZoomCenterAmount, obs_module_text("Zoom.CenterAmount"), 0.0, 1.0,
					0.01);
	obs_properties_add_group(props, "grp_zoom", obs_module_text("Group.Zoom"), OBS_GROUP_NORMAL, zoom);

	/* ---------------- Motion blur ---------------- */
	obs_properties_t *mb = obs_properties_create();
	obs_properties_add_bool(mb, kMotionBlurEnabled, obs_module_text("MotionBlur.Enabled"));
	obs_properties_add_float_slider(mb, kMotionBlurAmount, obs_module_text("MotionBlur.Amount"), 0.0, 1.0,
					0.01);
	obs_properties_add_group(props, "grp_motionblur", obs_module_text("Group.MotionBlur"), OBS_GROUP_NORMAL, mb);

	/* ---------------- Spotlight ---------------- */
	obs_properties_t *spot = obs_properties_create();
	obs_properties_add_bool(spot, kSpotlightEnabled, obs_module_text("Spotlight.Enabled"));
	obs_properties_add_float_slider(spot, kSpotlightRadius, obs_module_text("Spotlight.Radius"), 0.05, 1.0,
					0.01);
	obs_properties_add_float_slider(spot, kSpotlightDim, obs_module_text("Spotlight.Dim"), 0.0, 1.0, 0.01);
	obs_properties_add_group(props, "grp_spotlight", obs_module_text("Group.Spotlight"), OBS_GROUP_NORMAL,
				 spot);

	/* ---------------- Teclas ---------------- */
	obs_properties_t *keys = obs_properties_create();
	obs_properties_add_bool(keys, kKeysEnabled, obs_module_text("Keys.Enabled"));
	obs_properties_add_bool(keys, kKeysModifiersOnly, obs_module_text("Keys.ModifiersOnly"));
	obs_properties_add_float_slider(keys, kKeysTtlMs, obs_module_text("Keys.TtlMs"), 500.0, 8000.0, 50.0);
	obs_properties_add_int_slider(keys, kKeysMax, obs_module_text("Keys.Max"), 1, 12, 1);
	obs_properties_add_float_slider(keys, kKeysPosX, obs_module_text("Keys.PosX"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(keys, kKeysPosY, obs_module_text("Keys.PosY"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(keys, kKeysSize, obs_module_text("Keys.Size"), 0.4, 4.0, 0.05);

	obs_property_t *kfont = obs_properties_add_list(keys, kKeysFont, obs_module_text("Keys.Font"),
							OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	for (const auto &f : kFonts)
		obs_property_list_add_string(kfont, f[0], f[1]);

	obs_properties_add_color(keys, kKeysColor, obs_module_text("Keys.Color"));
	obs_properties_add_bool(keys, kKeysShadow, obs_module_text("Keys.Shadow"));
	obs_properties_add_bool(keys, kKeysBand, obs_module_text("Keys.Band"));
	obs_properties_add_color(keys, kKeysBandColor, obs_module_text("Keys.BandColor"));
	obs_properties_add_group(props, "grp_keys", obs_module_text("Group.Keys"), OBS_GROUP_NORMAL, keys);

	/* ---------------- Marca-texto ---------------- */
	obs_properties_t *hl = obs_properties_create();
	obs_properties_add_bool(hl, kHighlightEnabled, obs_module_text("Highlight.Enabled"));
	obs_properties_add_color(hl, kHighlightColor, obs_module_text("Highlight.Color"));
	obs_properties_add_float_slider(hl, kHighlightHoldMs, obs_module_text("Highlight.HoldMs"), 0.0, 5000.0,
					50.0);
	obs_properties_add_float_slider(hl, kHighlightFadeMs, obs_module_text("Highlight.FadeMs"), 0.0, 2000.0,
					50.0);
	obs_properties_add_group(props, "grp_highlight", obs_module_text("Group.Highlight"), OBS_GROUP_NORMAL,
				 hl);

	/* ---------------- Perfis ---------------- */
	obs_properties_t *prof = obs_properties_create();
	obs_property_t *plist = obs_properties_add_list(prof, kProfileCurrent, obs_module_text("Profile.Current"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(plist, obs_module_text("Profile.A"), 0);
	obs_property_list_add_int(plist, obs_module_text("Profile.B"), 1);
	obs_property_list_add_int(plist, obs_module_text("Profile.C"), 2);
	obs_property_list_add_int(plist, obs_module_text("Profile.D"), 3);
	obs_properties_add_group(props, "grp_profiles", obs_module_text("Group.Profiles"), OBS_GROUP_NORMAL, prof);

	return props;
}

} // namespace topscreen
