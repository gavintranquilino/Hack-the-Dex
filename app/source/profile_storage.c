#include "profile_storage.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

bool save_profile_socials(const char* directory, const char* discord,
                          const char* instagram, const char* twitter,
                          const char* linkedin) {
    char path[544], temporary_path[552];
    int count = snprintf(path, sizeof(path), "%s/profile.json", directory);
    if (count < 0 || (size_t)count >= sizeof(path)) return false;
    count = snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path);
    if (count < 0 || (size_t)count >= sizeof(temporary_path)) return false;

    FILE* file = fopen(path, "rb");
    if (!file) return false;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    char* data = malloc((size_t)length + 1);
    if (!data) { fclose(file); return false; }
    bool read_ok = fread(data, 1, (size_t)length, file) == (size_t)length;
    fclose(file);
    data[length] = '\0';
    cJSON* profile = read_ok ? cJSON_Parse(data) : NULL;
    free(data);
    if (!cJSON_IsObject(profile)) { cJSON_Delete(profile); return false; }

    const char* keys[] = { "discord", "instagram", "twitter", "linkedin" };
    const char* values[] = { discord, instagram, twitter, linkedin };
    for (int i = 0; i < 4; i++) {
        cJSON* value = cJSON_CreateString(values[i]);
        bool updated = value && (cJSON_HasObjectItem(profile, keys[i])
            ? cJSON_ReplaceItemInObjectCaseSensitive(profile, keys[i], value)
            : cJSON_AddItemToObject(profile, keys[i], value));
        if (!updated) {
            cJSON_Delete(value);
            cJSON_Delete(profile);
            return false;
        }
    }
    char* updated = cJSON_Print(profile);
    cJSON_Delete(profile);
    if (!updated) return false;

    // Keep the existing file intact until the complete update has been written.
    file = fopen(temporary_path, "wb");
    if (!file) { cJSON_free(updated); return false; }
    size_t size = strlen(updated);
    bool saved = fwrite(updated, 1, size, file) == size;
    if (fclose(file) != 0) saved = false;
    cJSON_free(updated);
    if (!saved) return false;
    if (rename(temporary_path, path) == 0) return true;
    if (errno != EEXIST) return false;

    // FatFs cannot rename over an existing file. Keep numbered backups rather
    // than deleting the old profile, and restore it if installation fails.
    char backup_path[568];
    for (unsigned int i = 1; i <= 1000; i++) {
        snprintf(backup_path, sizeof(backup_path), "%s.bak.%u", path, i);
        struct stat info;
        if (stat(backup_path, &info) == 0) continue;
        if (errno != ENOENT) return false;
        if (rename(path, backup_path) != 0) return false;
        if (rename(temporary_path, path) == 0) return true;
        // On a second I/O failure the backup still holds the original JSON.
        rename(backup_path, path);
        return false;
    }
    return false;
}
