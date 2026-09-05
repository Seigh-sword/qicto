#include "mod_builtins.h"
#include "syntax_mod.h"
#include "statusbar_mod.h"
#include "filetree_mod.h"
#include "module.h"
#include "editor.h"
#include <stdlib.h>
#include <string.h>

static void register_builtin(editor_t* ed, const qicto_mod_api_t* api) {
    if (!ed || !api || !api->name) return;
    if (!ed->mods) {
        ed->mods = mod_registry_create();
    }
    mod_entry_t* entry = calloc(1, sizeof(mod_entry_t));
    if (!entry) return;
    memcpy(&entry->api, api, sizeof(qicto_mod_api_t));
    strncpy(entry->name, api->name, QICTO_MAX_MOD_NAME - 1);
    entry->name[QICTO_MAX_MOD_NAME - 1] = '\0';
    entry->handle = NULL;
    entry->ref_count = 1;
    mod_registry_add(ed->mods, entry);
}

void mod_builtins_register(editor_t* ed) {
    if (!ed) return;
    register_builtin(ed, syntax_mod_get_api());
    register_builtin(ed, statusbar_mod_get_api());
    register_builtin(ed, filetree_mod_get_api());
}
