#ifndef PROFILE_STORAGE_H
#define PROFILE_STORAGE_H

#include <stdbool.h>

/* Update only social fields, preserving all other JSON members. */
bool save_profile_socials(const char* directory, const char* discord,
                          const char* instagram, const char* twitter,
                          const char* linkedin);

#endif
