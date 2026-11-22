#define _POSIX_C_SOURCE 200809L
#include "profile_storage.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static int fail_install;
int __real_rename(const char* old_path, const char* new_path);

// Model the DS FatFs rule: rename cannot replace an existing file.
int __wrap_rename(const char* old_path, const char* new_path) {
    struct stat info;
    if (stat(new_path, &info) == 0) { errno = EEXIST; return -1; }
    if (fail_install && strstr(old_path, ".tmp")) { errno = EIO; return -1; }
    return __real_rename(old_path, new_path);
}

static void write_profile(const char* path, const char* contents) {
    FILE* file = fopen(path, "wb");
    assert(file);
    assert(fputs(contents, file) >= 0);
    assert(fclose(file) == 0);
}

static char* read_profile(const char* path) {
    FILE* file = fopen(path, "rb");
    assert(file);
    assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    assert(size >= 0);
    rewind(file);
    char* contents = malloc((size_t)size + 1);
    assert(contents);
    assert(fread(contents, 1, (size_t)size, file) == (size_t)size);
    contents[size] = '\0';
    fclose(file);
    return contents;
}

static void expect_string(cJSON* json, const char* key, const char* expected) {
    cJSON* value = cJSON_GetObjectItemCaseSensitive(json, key);
    assert(cJSON_IsString(value));
    assert(strcmp(value->valuestring, expected) == 0);
}

int main(void) {
    char directory[] = "/tmp/hackthedex-socials-XXXXXX";
    assert(mkdtemp(directory));
    char path[512], temporary[520];
    snprintf(path, sizeof(path), "%s/profile.json", directory);
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    const char* original = "{\"name\":\"Alex\",\"pronouns\":\"they/them\","
        "\"discord\":\"old\",\"photo\":\"custom.bmp\",\"signature\":\"mark.bmp\","
        "\"extra\":{\"id\":42},\"registered\":true}";
    write_profile(path, original);
    assert(save_profile_socials(directory, "user\\\"name", "", "@new", "a/b"));
    char* saved = read_profile(path);
    cJSON* json = cJSON_Parse(saved);
    assert(json);
    expect_string(json, "name", "Alex");
    expect_string(json, "pronouns", "they/them");
    expect_string(json, "photo", "custom.bmp");
    expect_string(json, "signature", "mark.bmp");
    expect_string(json, "discord", "user\\\"name");
    expect_string(json, "instagram", "");
    expect_string(json, "twitter", "@new");
    expect_string(json, "linkedin", "a/b");
    assert(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(json, "registered")));
    assert(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(json, "extra"), "id")->valueint == 42);
    cJSON_Delete(json);
    free(saved);

    char backup[568];
    snprintf(backup, sizeof(backup), "%s.bak.1", path);
    saved = read_profile(backup);
    assert(strcmp(saved, original) == 0);
    free(saved);

    // Repeated updates replace fields and persist across reloads.
    assert(save_profile_socials(directory, "second", "instagram", "", "linkedin"));
    saved = read_profile(path);
    json = cJSON_Parse(saved);
    expect_string(json, "discord", "second");
    expect_string(json, "twitter", "");
    cJSON_Delete(json);
    free(saved);

    // A failed install restores the original and reports failure.
    saved = read_profile(path);
    fail_install = 1;
    assert(!save_profile_socials(directory, "failed", "", "", ""));
    fail_install = 0;
    char* restored = read_profile(path);
    assert(strcmp(saved, restored) == 0);
    free(saved);
    free(restored);

    // Malformed or non-object JSON must remain untouched.
    const char* invalid[] = { "{broken", "[]", "null" };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        write_profile(path, invalid[i]);
        assert(!save_profile_socials(directory, "", "", "", ""));
        saved = read_profile(path);
        assert(strcmp(saved, invalid[i]) == 0);
        free(saved);
    }

    // A blocked temporary path simulates a write failure without truncating the original.
    write_profile(path, original);
    // Failed installation leaves its temporary file for recovery. A separate
    // directory lets us test a blocked write path without removing any files.
    char blocked_directory[] = "/tmp/hackthedex-socials-blocked-XXXXXX";
    assert(mkdtemp(blocked_directory));
    snprintf(path, sizeof(path), "%s/profile.json", blocked_directory);
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    write_profile(path, original);
    assert(mkdir(temporary, 0700) == 0);
    assert(!save_profile_socials(blocked_directory, "new", "", "", ""));
    saved = read_profile(path);
    assert(strcmp(saved, original) == 0);
    free(saved);
    assert(!save_profile_socials("/nonexistent/hackthedex", "", "", "", ""));
    puts("profile storage tests passed");
    return 0;
}
