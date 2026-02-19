// CallOfTheMoutains - UI Style System
// Dark Souls / Dystopian aesthetic - rusted metal, amber accents, worn textures

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"

/**
 * Centralized UI styling for CallOfTheMoutains
 * Design: Dark medieval dystopian - rusted iron, amber fire, cold stone
 */
namespace COTMStyle
{
	// ============================================================================
	// COLOR PALETTE - Dark Souls Dystopian
	// ============================================================================

	namespace Colors
	{
		// --- Backgrounds (Deep blacks with warm undertones) ---
		inline FLinearColor BackgroundDark()      { return FLinearColor(0.012f, 0.01f, 0.008f, 0.92f); }
		inline FLinearColor BackgroundPanel()     { return FLinearColor(0.035f, 0.03f, 0.025f, 0.88f); }
		inline FLinearColor BackgroundSlot()      { return FLinearColor(0.055f, 0.05f, 0.04f, 0.85f); }
		inline FLinearColor BackgroundHover()     { return FLinearColor(0.08f, 0.07f, 0.05f, 0.9f); }

		// --- Borders (Rusted metal tones) ---
		inline FLinearColor BorderRust()          { return FLinearColor(0.45f, 0.25f, 0.1f, 0.9f); }
		inline FLinearColor BorderRustDark()      { return FLinearColor(0.25f, 0.14f, 0.06f, 0.85f); }
		inline FLinearColor BorderRustLight()     { return FLinearColor(0.6f, 0.35f, 0.15f, 0.95f); }
		inline FLinearColor BorderIron()          { return FLinearColor(0.18f, 0.17f, 0.16f, 0.8f); }
		inline FLinearColor BorderIronLight()     { return FLinearColor(0.28f, 0.26f, 0.24f, 0.85f); }

		// --- Accents (Ember/Fire tones) ---
		inline FLinearColor AccentAmber()         { return FLinearColor(0.95f, 0.65f, 0.15f, 0.9f); }
		inline FLinearColor AccentEmber()         { return FLinearColor(1.0f, 0.45f, 0.1f, 0.85f); }
		inline FLinearColor AccentGold()          { return FLinearColor(0.85f, 0.75f, 0.35f, 0.9f); }
		inline FLinearColor AccentCold()          { return FLinearColor(0.4f, 0.55f, 0.65f, 0.8f); }

		// --- Text Colors ---
		inline FLinearColor TextPrimary()         { return FLinearColor(0.88f, 0.85f, 0.78f, 0.95f); }
		inline FLinearColor TextSecondary()       { return FLinearColor(0.6f, 0.55f, 0.48f, 0.85f); }
		inline FLinearColor TextMuted()           { return FLinearColor(0.4f, 0.38f, 0.35f, 0.7f); }
		inline FLinearColor TextHighlight()       { return FLinearColor(1.0f, 0.85f, 0.5f, 1.0f); }

		// --- Rarity Colors ---
		inline FLinearColor RarityCommon()        { return FLinearColor(0.5f, 0.48f, 0.45f, 0.8f); }
		inline FLinearColor RarityUncommon()      { return FLinearColor(0.35f, 0.65f, 0.35f, 0.85f); }
		inline FLinearColor RarityRare()          { return FLinearColor(0.3f, 0.5f, 0.8f, 0.9f); }
		inline FLinearColor RarityEpic()          { return FLinearColor(0.6f, 0.3f, 0.7f, 0.9f); }
		inline FLinearColor RarityLegendary()     { return FLinearColor(0.95f, 0.7f, 0.2f, 0.95f); }

		// --- Slot States ---
		inline FLinearColor SlotEmpty()           { return FLinearColor(0.04f, 0.035f, 0.03f, 0.6f); }
		inline FLinearColor SlotFilled()          { return FLinearColor(0.06f, 0.055f, 0.045f, 0.75f); }
		inline FLinearColor SlotSelected()        { return FLinearColor(0.12f, 0.08f, 0.04f, 0.85f); }

		// --- Effects ---
		inline FLinearColor GlowInner()           { return FLinearColor(0.4f, 0.25f, 0.1f, 0.4f); }
		inline FLinearColor GlowOuter()           { return FLinearColor(0.2f, 0.12f, 0.05f, 0.25f); }
		inline FLinearColor Shadow()              { return FLinearColor(0.0f, 0.0f, 0.0f, 0.5f); }

		// --- Stat Bars (Health/Stamina) ---
		inline FLinearColor HealthFill()          { return FLinearColor(0.7f, 0.12f, 0.08f, 1.0f); }
		inline FLinearColor HealthCritical()      { return FLinearColor(0.5f, 0.05f, 0.05f, 1.0f); }
		inline FLinearColor HealthBackground()    { return FLinearColor(0.15f, 0.03f, 0.02f, 0.9f); }
		inline FLinearColor StaminaFill()         { return FLinearColor(0.25f, 0.55f, 0.45f, 1.0f); }
		inline FLinearColor StaminaDepleted()     { return FLinearColor(0.12f, 0.25f, 0.2f, 1.0f); }
		inline FLinearColor StaminaBackground()   { return FLinearColor(0.04f, 0.08f, 0.07f, 0.85f); }

		// --- Faith Currency ---
		inline FLinearColor FaithGlow()           { return FLinearColor(0.95f, 0.75f, 0.25f, 1.0f); }
		inline FLinearColor FaithCore()           { return FLinearColor(1.0f, 0.85f, 0.4f, 1.0f); }
		inline FLinearColor TextGain()            { return FLinearColor(0.4f, 0.9f, 0.4f, 1.0f); }
		inline FLinearColor TextLoss()            { return FLinearColor(0.9f, 0.3f, 0.2f, 1.0f); }

		// --- Frame Colors (for bars) ---
		inline FLinearColor FrameRust()           { return FLinearColor(0.35f, 0.2f, 0.1f, 0.95f); }
		inline FLinearColor FrameIronDark()       { return FLinearColor(0.15f, 0.12f, 0.1f, 0.9f); }
		inline FLinearColor FrameHighlight()      { return FLinearColor(0.5f, 0.3f, 0.15f, 0.6f); }
	}

	// ============================================================================
	// FONTS
	// ============================================================================

	namespace Fonts
	{
		inline FSlateFontInfo Header()    { return FCoreStyle::GetDefaultFontStyle("Bold", 28); }
		inline FSlateFontInfo SubHeader() { return FCoreStyle::GetDefaultFontStyle("Bold", 20); }
		inline FSlateFontInfo Body()      { return FCoreStyle::GetDefaultFontStyle("Regular", 16); }
		inline FSlateFontInfo Small()     { return FCoreStyle::GetDefaultFontStyle("Regular", 14); }
		inline FSlateFontInfo Tiny()      { return FCoreStyle::GetDefaultFontStyle("Regular", 11); }
		inline FSlateFontInfo KeyHint()   { return FCoreStyle::GetDefaultFontStyle("Bold", 12); }
		inline FSlateFontInfo Quantity()  { return FCoreStyle::GetDefaultFontStyle("Bold", 16); }
	}

	// ============================================================================
	// LAYOUT
	// ============================================================================

	namespace Layout
	{
		inline float BorderThin()       { return 1.0f; }
		inline float BorderMedium()     { return 2.0f; }
		inline float BorderThick()      { return 3.0f; }
		inline float PaddingSmall()     { return 3.0f; }
		inline float PaddingMedium()    { return 6.0f; }
		inline float PaddingLarge()     { return 12.0f; }
	}
}
