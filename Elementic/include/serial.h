/*
 * serial.h
 *
 * Declarations for the serial protocol interface used to configure, synchronize, and exchange data with Elementic clients.
 */

#pragma once

#include <Arduino.h>

void serialsetup();
void serialloop();
void serialwrite(byte b);
