/*
 * mock_afc_mobileactivation_ctl.c -- Staging + reset for the AFC /
 * mobileactivation mock.  Kept separate from _sym.c so each TU stays
 * under the 300-line project cap.
 */

#include "mock_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Shared with the sym file via extern declarations in the header below. */
int  g_ma_activate_error;
int  g_ma_create_info_error;
char g_ma_state[64] = "Activated";

void mock_mobileactivation_set_activate_error(int error_code)
{
    g_ma_activate_error = error_code;
}

void mock_mobileactivation_set_create_info_error(int error_code)
{
    g_ma_create_info_error = error_code;
}

void mock_mobileactivation_set_state(const char *state)
{
    if (!state)
        snprintf(g_ma_state, sizeof(g_ma_state), "%s", "Activated");
    else
        snprintf(g_ma_state, sizeof(g_ma_state), "%s", state);
}

void mock_afc_mobileactivation_reset(void)
{
    g_ma_activate_error    = 0;
    g_ma_create_info_error = 0;
    snprintf(g_ma_state, sizeof(g_ma_state), "%s", "Activated");
}
