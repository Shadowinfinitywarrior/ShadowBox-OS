#pragma once

#include <memory>
#include <stdint.h>
#include "gui_public.h"

// Central GUI manager — owns compositor, input router, and widget tree
// Reduces boilerplate in demo applications and future apps.
// Provides a clean C-compatible API (via gui_public.h) and C++ convenience.

extern class GUIManager;

extern GUIManager gui_manager;
