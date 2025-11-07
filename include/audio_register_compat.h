#ifndef AUDIO_REGISTER_COMPAT_H
#define AUDIO_REGISTER_COMPAT_H

// Register name shims for the Teensy Audio Library across core releases.
//
// The Audio library historically used the MK20/MK66 "I2S0_*" register names and
// routing macros. Teensy 4.x boards (IMXRT1062) renamed the same peripherals to
// SAI1/I2S3 and tweaked a few helper macros. When PlatformIO pulls in a newer
// Teensyduino core the legacy names disappear, which torpedoes builds that rely
// on older Audio releases. Rather than pinning the entire toolchain we patch the
// missing symbols here and forward them to their modern equivalents.
//
// The header is force-included from platformio.ini so every translation unit in
// the Audio library sees the aliases before it includes its own headers.

#ifdef ARDUINO
#include <Arduino.h>
#endif

#if defined(__IMXRT1062__)
#include <imxrt.h>

// --- Pin mux helpers -------------------------------------------------------
// The legacy Audio code expects Kinetis-style PORT mux helpers. Teensy 4's core
// exposes similar helpers through the IOMUXC block, so we simply forward the
// macro when available. Falling back to the bare mux value keeps the older code
// compiling even if the helper macro changes upstream.
#ifndef PORT_PCR_MUX
#if defined(IOMUXC_SW_MUX_CTL_PAD_MUX_MODE)
#define PORT_PCR_MUX(n) (IOMUXC_SW_MUX_CTL_PAD_MUX_MODE((n)))
#else
#define PORT_PCR_MUX(n) ((n) & 0x7)
#endif
#endif

// --- DMA routing aliases ---------------------------------------------------
#if !defined(DMAMUX_SOURCE_I2S0_RX) && defined(DMAMUX_SOURCE_SAI1_RX)
#define DMAMUX_SOURCE_I2S0_RX DMAMUX_SOURCE_SAI1_RX
#endif

#if !defined(DMAMUX_SOURCE_I2S0_TX) && defined(DMAMUX_SOURCE_SAI1_TX)
#define DMAMUX_SOURCE_I2S0_TX DMAMUX_SOURCE_SAI1_TX
#endif

// --- I2S register aliases --------------------------------------------------
// The IMXRT core exposes the I2S controller as instance 3. Mirror the original
// register names so the library continues to build without surgery.
#ifndef I2S0_RCSR
#define I2S0_RCSR I2S3_RCSR
#endif
#ifndef I2S0_RCR1
#define I2S0_RCR1 I2S3_RCR1
#endif
#ifndef I2S0_RCR2
#define I2S0_RCR2 I2S3_RCR2
#endif
#ifndef I2S0_RCR3
#define I2S0_RCR3 I2S3_RCR3
#endif
#ifndef I2S0_RCR4
#define I2S0_RCR4 I2S3_RCR4
#endif
#ifndef I2S0_RCR5
#define I2S0_RCR5 I2S3_RCR5
#endif
#ifndef I2S0_RDR0
#define I2S0_RDR0 I2S3_RDR0
#endif
#ifndef I2S0_RMR
#define I2S0_RMR I2S3_RMR
#endif
#ifndef I2S0_TCSR
#define I2S0_TCSR I2S3_TCSR
#endif
#ifndef I2S0_TCR1
#define I2S0_TCR1 I2S3_TCR1
#endif
#ifndef I2S0_TCR2
#define I2S0_TCR2 I2S3_TCR2
#endif
#ifndef I2S0_TCR3
#define I2S0_TCR3 I2S3_TCR3
#endif
#ifndef I2S0_TCR4
#define I2S0_TCR4 I2S3_TCR4
#endif
#ifndef I2S0_TCR5
#define I2S0_TCR5 I2S3_TCR5
#endif
#ifndef I2S0_TDR0
#define I2S0_TDR0 I2S3_TDR0
#endif
#ifndef I2S0_TMR
#define I2S0_TMR I2S3_TMR
#endif
#ifndef I2S0_MCR
#define I2S0_MCR I2S3_MCR
#endif
#ifndef I2S0_MDR
#define I2S0_MDR I2S3_MDR
#endif

#endif  // __IMXRT1062__

#endif  // AUDIO_REGISTER_COMPAT_H
