#include "core/profiles.hpp"
#include "test_framework.hpp"

#include <string>

using namespace topscreen::core;

TEST(profiles_seed_a_e_padrao)
{
	const Config a = make_profile_seed(0);
	const Config def;
	CHECK_EQ(a.click_left.color, def.click_left.color);
	CHECK_NEAR(a.zoom.amount, def.zoom.amount, 1e-9);
	CHECK_EQ(a.zoom.mode, def.zoom.mode);
}

TEST(profiles_seed_b_muda_cor_e_zoom)
{
	const Config b = make_profile_seed(1);
	CHECK_EQ(b.click_left.color, 0xFF00FF00u);
	CHECK_EQ(b.click_right.color, 0xFF00A5FFu);
	CHECK_NEAR(b.zoom.amount, 2.0, 1e-9);
}

TEST(profiles_seed_c_e_d)
{
	const Config c = make_profile_seed(2);
	CHECK_EQ(c.click_left.color, 0xFFFF5C7Cu);
	CHECK_NEAR(c.zoom.amount, 1.4, 1e-9);
	CHECK_EQ(c.bg.mode, static_cast<int>(BgMode::Vertical));

	const Config d = make_profile_seed(3);
	CHECK_EQ(d.zoom.mode, static_cast<int>(ZoomMode::Off));
	CHECK_EQ(d.click_left.color, 0xFF00D6FFu);
}

TEST(profiles_seed_diferentes_entre_si)
{
	/* Nenhum par de slots pode ser identico (senao a troca seria invisivel). */
	for (int i = 0; i < 4; ++i) {
		for (int j = i + 1; j < 4; ++j) {
			const Config a = make_profile_seed(i);
			const Config b = make_profile_seed(j);
			const bool same = a.click_left.color == b.click_left.color &&
					  a.click_right.color == b.click_right.color &&
					  a.zoom.amount == b.zoom.amount && a.zoom.mode == b.zoom.mode;
			CHECK(!same);
		}
	}
}

TEST(profiles_seed_indice_fora_da_faixa)
{
	/* Nao deve quebrar e deve ciclar. */
	const Config e = make_profile_seed(4);
	const Config a = make_profile_seed(0);
	CHECK_EQ(e.click_left.color, a.click_left.color);
}

TEST(profiles_rotulos_distintos)
{
	CHECK_EQ(std::string(profile_label(0)), std::string("Perfil A"));
	CHECK_EQ(std::string(profile_label(1)), std::string("Perfil B"));
	CHECK_EQ(std::string(profile_label(2)), std::string("Perfil C"));
	CHECK_EQ(std::string(profile_label(3)), std::string("Perfil D"));

	/* Nunca repetidos (era a reclamacao: "todos ainda perfil"). */
	for (int i = 0; i < 4; ++i)
		for (int j = i + 1; j < 4; ++j)
			CHECK(std::string(profile_label(i)) != std::string(profile_label(j)));

	CHECK_EQ(std::string(profile_label(5)), std::string("Perfil B"));
	CHECK_EQ(profile_index(-1), 3);
}
