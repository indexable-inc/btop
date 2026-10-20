// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <optional>
#include <string_view>

#include <gtest/gtest.h>

#include "btop_theme.hpp"

using Theme::Background;

namespace {
	Background classify(std::string_view reply) {
		const auto rgb = Theme::parse_osc11_reply(reply);
		EXPECT_TRUE(rgb.has_value()) << "unparsed reply";
		return Theme::classify_background(Theme::relative_luminance(rgb.value_or(std::array<double, 3>{})));
	}
}

TEST(theme, osc11_reply_parses_bel_and_st_terminators) {
	const auto bel = Theme::parse_osc11_reply("\x1b]11;rgb:ffff/8080/0000\a");
	ASSERT_TRUE(bel.has_value());
	EXPECT_DOUBLE_EQ((*bel)[0], 1.0);
	EXPECT_DOUBLE_EQ((*bel)[1], 0x8080 / 65535.0);
	EXPECT_DOUBLE_EQ((*bel)[2], 0.0);

	const auto st = Theme::parse_osc11_reply("\x1b]11;rgb:0000/0000/ffff\x1b\\");
	ASSERT_TRUE(st.has_value());
	EXPECT_DOUBLE_EQ((*st)[2], 1.0);
}

TEST(theme, osc11_reply_scales_short_components) {
	const auto rgb = Theme::parse_osc11_reply("\x1b]11;rgb:f/80/fff\a");
	ASSERT_TRUE(rgb.has_value());
	EXPECT_DOUBLE_EQ((*rgb)[0], 1.0);
	EXPECT_DOUBLE_EQ((*rgb)[1], 0x80 / 255.0);
	EXPECT_DOUBLE_EQ((*rgb)[2], 1.0);
}

TEST(theme, osc11_reply_found_before_device_attributes) {
	//? The reply buffer holds the OSC 11 answer followed by the device attributes answer
	EXPECT_TRUE(Theme::parse_osc11_reply("\x1b]11;rgba:2828/2828/2828/ffff\x1b\\\x1b[?62;22c").has_value());
}

TEST(theme, osc11_reply_rejects_incomplete_or_malformed) {
	EXPECT_FALSE(Theme::parse_osc11_reply("").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b[?62;22c").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b]11;rgb:ffff/ffff/ffff").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b]11;rgb:ffff/ffff/ffff\x1b").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b]11;rgb:ffff/ffff\a").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b]11;rgb:ffff/ffff/ffff/ffff\a").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b]11;rgb:fffff/0/0\a").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b]11;rgb:zz/00/00\a").has_value());
	EXPECT_FALSE(Theme::parse_osc11_reply("\x1b]11;#ffffff\a").has_value());
}

TEST(theme, terminal_backgrounds_classify_by_contrast) {
	EXPECT_EQ(classify("\x1b]11;rgb:0000/0000/0000\a"), Background::Dark);
	EXPECT_EQ(classify("\x1b]11;rgb:ffff/ffff/ffff\a"), Background::Light);
	//? gruvbox dark bg0 #282828 and light bg0 #fbf1c7
	EXPECT_EQ(classify("\x1b]11;rgb:2828/2828/2828\a"), Background::Dark);
	EXPECT_EQ(classify("\x1b]11;rgb:fbfb/f1f1/c7c7\a"), Background::Light);
	//? Solarized base03 #002b36 and base3 #fdf6e3
	EXPECT_EQ(classify("\x1b]11;rgb:0000/2b2b/3636\a"), Background::Dark);
	EXPECT_EQ(classify("\x1b]11;rgb:fdfd/f6f6/e3e3\a"), Background::Light);
}

TEST(theme, luminance_threshold_is_equal_contrast_point) {
	//? Black and white text have equal contrast at L = sqrt(1.05 * 0.05) - 0.05
	const double threshold = 0.17912878474779197;
	EXPECT_EQ(Theme::classify_background(threshold - 1e-6), Background::Dark);
	EXPECT_EQ(Theme::classify_background(threshold + 1e-6), Background::Light);
	//? The greys either side of it: #757575 (L ~0.178) and #767676 (L ~0.181)
	EXPECT_EQ(classify("\x1b]11;rgb:7575/7575/7575\a"), Background::Dark);
	EXPECT_EQ(classify("\x1b]11;rgb:7676/7676/7676\a"), Background::Light);
}
