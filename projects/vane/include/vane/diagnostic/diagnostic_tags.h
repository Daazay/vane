#pragma once

#define DIAG_DRIVER  "driver"
#define DIAG_SCANNER "scanner"
#define DIAG_PARSER  "parser"
#define DIAG_SEMA    "sema"

/* Driver subdomains */
#define DIAG_DRIVER_FS      DIAG_DRIVER":fs"      // file IO, path, directory walking
#define DIAG_DRIVER_IMPORTS DIAG_DRIVER":imports" // resolving import paths / collections
#define DIAG_DRIVER_CONFIG  DIAG_DRIVER":config"  // CLI/options/env
#define DIAG_DRIVER_PROJECT DIAG_DRIVER":project" // project discovery / package graph

/* Scanner/Parser */
#define DIAG_SCANNER_TOK DIAG_SCANNER":token"
#define DIAG_PARSER_TOK  DIAG_PARSER":syntax"

/* Sema subdomains */
#define DIAG_SEMA_SYMBOLS DIAG_SEMA":symbols" // decl duplications, shadowing, scope rules
#define DIAG_SEMA_BIND    DIAG_SEMA":bind"    // name binding in expression
#define DIAG_SEMA_IMPORTS DIAG_SEMA":imports" // alias imports, opened imports, qualified access
#define DIAG_SEMA_TYPES   DIAG_SEMA":types"   // type names, typerefs, cyclic/type resolution