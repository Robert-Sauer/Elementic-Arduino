/*
 * ElementicLimits.h
 *
 * Compile-time SRAM allocation limits. Elementic-generated projects define
 * ELEMENTIC_MAX_INPUTS and ELEMENTIC_MAX_OUTPUTS before this file is included.
 * Manual sketches use the platform defaults below and may override either the
 * canonical ELEMENTIC_* names or the historical InputChannels/OutputChannels
 * names before including Elementic.h.
 */

#pragma once

#if defined(ARDUINO_ARCH_AVR)
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_INPUTS 19
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_OUTPUTS 23
#elif defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_INPUTS 19
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_OUTPUTS 23
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_INPUTS 19
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_OUTPUTS 23
#elif defined(ARDUINO_ARCH_RP2040)
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_INPUTS 19
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_OUTPUTS 23
#else
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_INPUTS 19
  #define ELEMENTIC_PLATFORM_DEFAULT_MAX_OUTPUTS 23
#endif

// Preserve manual sketches that already define the historical names.
#if defined(InputChannels) && !defined(ELEMENTIC_MAX_INPUTS)
  #define ELEMENTIC_MAX_INPUTS InputChannels
#endif
#if defined(OutputChannels) && !defined(ELEMENTIC_MAX_OUTPUTS)
  #define ELEMENTIC_MAX_OUTPUTS OutputChannels
#endif

#ifndef ELEMENTIC_MAX_INPUTS
  #define ELEMENTIC_MAX_INPUTS ELEMENTIC_PLATFORM_DEFAULT_MAX_INPUTS
#endif
#ifndef ELEMENTIC_MAX_OUTPUTS
  #define ELEMENTIC_MAX_OUTPUTS ELEMENTIC_PLATFORM_DEFAULT_MAX_OUTPUTS
#endif

#if ELEMENTIC_MAX_INPUTS < 1 || ELEMENTIC_MAX_INPUTS > 254
  #error "ELEMENTIC_MAX_INPUTS must be in the range 1..254"
#endif
#if ELEMENTIC_MAX_OUTPUTS < 1 || ELEMENTIC_MAX_OUTPUTS > 254
  #error "ELEMENTIC_MAX_OUTPUTS must be in the range 1..254"
#endif

#ifndef InputChannels
  #define InputChannels ELEMENTIC_MAX_INPUTS
#elif InputChannels != ELEMENTIC_MAX_INPUTS
  #error "InputChannels and ELEMENTIC_MAX_INPUTS must have the same value"
#endif

#ifndef OutputChannels
  #define OutputChannels ELEMENTIC_MAX_OUTPUTS
#elif OutputChannels != ELEMENTIC_MAX_OUTPUTS
  #error "OutputChannels and ELEMENTIC_MAX_OUTPUTS must have the same value"
#endif
