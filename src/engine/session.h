#ifndef QICTO_SESSION_H
#define QICTO_SESSION_H

#include "qicto.h"

#define QICTO_SESSION_VERSION 1

/* Find the project directory for the given starting dir by walking
 * up looking for a .qicto/project.json. Returns a path that the
 * caller must free, or NULL. */
char* session_find_project_dir(const char* start_dir);

/* Save the current editor state to .qicto/project.json inside the
 * given project directory. Returns 0 on success, non-zero on error. */
int session_save(editor_t* ed, const char* project_dir);

/* Load a project file from project_dir/.qicto/project.json, opening
 * every buffer it lists. The first buffer becomes the current one.
 * Returns 0 on success, non-zero if no project file or parse error. */
int session_load(editor_t* ed, const char* project_dir);

#endif
