#ifndef KINETIS_COMPAT_SHIM_H
#define KINETIS_COMPAT_SHIM_H

// Compatibility shim so legacy Teensy Audio sources that blindly include
// <kinetis.h> keep building when the PlatformIO toolchain pulls in an IMXRT
// (Teensy 4.x) core. The actual Kinetis register map is irrelevant on this
// target; we just need a header that funnels the include over to the modern
// IMXRT definitions without exploding the build.

#if defined(__IMXRT1062__)
#include <imxrt.h>
#else
#error "This kinetis.h shim only targets the IMXRT-based Teensy 4 family."
#endif

#endif  // KINETIS_COMPAT_SHIM_H
