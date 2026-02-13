#!/usr/bin/env python3
# Copyright (c) 2026 honsma235
# SPDX-License-Identifier: GPL-2.0-only
#
# See the repository LICENSE file for the full text.

## todo: multiple parameters, array parameters

"""
SCPI Artifact Generator: YAML -> Markdown reference + C handler code

Generates all artifacts in the same directory as the input YAML file:
  1. SCPI_COMMANDS.md - Markdown documentation
  2. scpi_commands_gen.h - C header with parameter structures
  3. scpi_commands_gen.c - C implementation with handlers

Usage:
  python tools/generate_scpi_artifacts.py scpi_server/scpi_commands.yaml
  python tools/generate_scpi_artifacts.py scpi_server/scpi_commands.yaml --force

Requires: PyYAML (`pip install pyyaml`)
"""
import sys
import os
import argparse
import re
import time

try:
    import yaml
except Exception:
    print("Please install PyYAML: pip install pyyaml", file=sys.stderr)
    raise

# Global mapping: generated enum type name -> canonical (first) enum type name
# Populated during gen_enums() to handle deduplication
_ENUM_TYPE_MAPPING = {}


def safe_ident(cmd):
    """Convert SCPI command to valid C identifier"""
    # Remove brackets and placeholders, replace special chars with underscore
    s = re.sub(r"[<>?]", "", cmd)  # Remove <>, ?
    s = re.sub(r"[^0-9a-zA-Z]+", "_", s)
    s = s.strip("_")
    return s.upper()


def is_query(cmd):
    """Check if command is a query (ends with ?)"""
    return cmd.endswith("?")


def get_param_type_c(ptype):
    """Map YAML param type to C type"""
    type_map = {
        "int": "int",
        "uint": "unsigned int",
        "float": "float",
        "bool": "bool",
        "boolean": "bool",
        "enum": "int",  # Will be mapped to enum later
        "string": "const char*",
    }
    return type_map.get(ptype, "const char*")


def get_enum_type_name(ident, param_name):
    """Generate enum type name for enum parameters"""
    return f"{ident}_{param_name.upper()}_t"


def get_param_type_c_for_param(ident, param):
    """Get the proper C type for a parameter (uses enum typedef if type is enum).
    Uses the deduplication mapping if available."""
    ptype = param.get("type", "")
    if ptype == "enum":
        param_name = param.get("name", "")
        enum_name = get_enum_type_name(ident, param_name)
        # If this enum was deduplicated to another name, use the canonical one
        return _ENUM_TYPE_MAPPING.get(enum_name, enum_name)
    return get_param_type_c(ptype)


def generate_param_parsing_code(ident, params):
    """Generate parameter parsing code lines for a handler.
    Returns list of code lines."""
    lines = []
    if not params:
        return lines
    
    lines.append("    /* Parse and validate parameters */")
    for param in params:
        param_name = param.get("name", "")
        ptype = param.get("type", "")
        pmin = param.get("min")
        pmax = param.get("max")
        pvals = param.get("values", [])
        has_min = pmin is not None
        has_max = pmax is not None
        min_val = pmin if has_min else 0
        max_val = pmax if has_max else 0
        
        if ptype == "int":
            lines.append(f"    if (scpi_gen_parse_int(context, &{param_name}, {'true' if has_min else 'false'}, {min_val}, {'true' if has_max else 'false'}, {max_val}) != SCPI_RES_OK) {{")
            lines.append(f"        return SCPI_RES_ERR;")
            lines.append(f"    }}")
            
        elif ptype == "uint":
            lines.append(f"    if (scpi_gen_parse_uint(context, &{param_name}, {'true' if has_min else 'false'}, {min_val}, {'true' if has_max else 'false'}, {max_val}) != SCPI_RES_OK) {{")
            lines.append(f"        return SCPI_RES_ERR;")
            lines.append(f"    }}")
            
        elif ptype == "float":
            lines.append(f"    if (scpi_gen_parse_float(context, &{param_name}, {'true' if has_min else 'false'}, {min_val}, {'true' if has_max else 'false'}, {max_val}) != SCPI_RES_OK) {{")
            lines.append(f"        return SCPI_RES_ERR;")
            lines.append(f"    }}")
            
        elif ptype == "bool" or ptype == "boolean":
            lines.append(f"    if (scpi_gen_parse_bool(context, &{param_name}) != SCPI_RES_OK) {{")
            lines.append(f"        return SCPI_RES_ERR;")
            lines.append(f"    }}")
            
        elif ptype == "enum":
            enum_name = get_enum_type_name(ident, param_name)
            # Use mapped canonical enum type if available
            canonical_enum = _ENUM_TYPE_MAPPING.get(enum_name, enum_name)
            choice_var = f"source_{safe_ident(param_name)}_choices"
            lines.append(f"    /* Parse {param_name} (enum) */")
            lines.append(f"    {{")
            lines.append(f"        int32_t {param_name}_value = 0;")
            lines.append(f"        if (!SCPI_ParamChoice(context, {choice_var}, &{param_name}_value, TRUE)) {{")
            lines.append(f"            return SCPI_RES_ERR;")
            lines.append(f"        }}")
            lines.append(f"        {param_name} = ({canonical_enum}){param_name}_value;")
            lines.append(f"    }}")
        
        else:
            lines.append(f"    /* TODO: Parse {param_name} ({ptype}) */")
    
    lines.append("")
    return lines


def generate_response_formatting_code(ident, params):
    """Generate response formatting code lines for a query handler.
    Returns list of code lines."""
    lines = []
    if not params:
        return lines
    
    lines.append("    /* Format response based on parameter types */")
    for param in params:
        param_name = param.get("name", "")
        ptype = param.get("type", "")
        
        if ptype == "int":
            lines.append(f"    SCPI_ResultInt32(context, (int32_t){param_name});")
        elif ptype == "uint":
            lines.append(f"    SCPI_ResultUInt32(context, (uint32_t){param_name});")
        elif ptype == "float":
            lines.append(f"    SCPI_ResultFloat(context, {param_name});")
        elif ptype == "bool" or ptype == "boolean":
            lines.append(f"    SCPI_ResultBool(context, {param_name});")
        elif ptype == "enum":
            choice_var = f"source_{safe_ident(param_name)}_choices"
            lines.append(f"    /* Format enum as string */")
            lines.append(f"    {{")
            lines.append(f"        const char *name;")
            lines.append(f"        if (SCPI_ChoiceToName({choice_var}, (int32_t){param_name}, &name)) {{")
            lines.append(f"            SCPI_ResultMnemonic(context, name);")
            lines.append(f"        }}")
            lines.append(f"    }}")
    
    lines.append("")
    return lines


def generate_index_extraction_code(indices):
    """Generate index extraction and validation code lines.
    Returns list of code lines."""
    lines = []
    if not indices:
        return lines
    
    lines.append("    /* Extract and validate indices */")
    idx_mins = []
    idx_maxs = []
    for index_info in indices:
        idx_range = index_info.get("range", "1-10")
        idx_min, idx_max = parse_index_range(idx_range)
        idx_mins.append(str(idx_min))
        idx_maxs.append(str(idx_max))
    lines.append(f"    if (scpi_gen_parse_indices(context, indices, (const unsigned int[]){{{', '.join(idx_mins)}}}, (const unsigned int[]){{{', '.join(idx_maxs)}}}, {len(indices)}) != SCPI_RES_OK) {{")
    lines.append(f"        return SCPI_RES_ERR;")
    lines.append(f"    }}")
    lines.append("")
    return lines


def parse_index_range(range_str):
    """Parse index range like '1-3' into (min, max)"""
    parts = range_str.split('-')
    if len(parts) == 2:
        return int(parts[0]), int(parts[1])
    return None, None


def has_enum_params(params):
    """Check if any parameter is enum type"""
    return any(p.get("type") == "enum" for p in params)


def has_multiple_params(params):
    """Check if command has multiple parameters"""
    return len(params) > 1


def generate_enum_choices_code(cmd_entry):
    """Generate choice list definitions for enum parameters in a command.
    Returns list of code lines."""
    lines = []
    cmd = cmd_entry.get("command", "")
    params = cmd_entry.get("params", [])
    
    if is_query(cmd):
        return lines
    
    ident = safe_ident(cmd)
    
    for param in params:
        if param.get("type") == "enum":
            param_name = param.get("name", "")
            values = param.get("values", [])
            choice_var = f"source_{safe_ident(param_name)}_choices"
            
            lines.append(f"static const scpi_choice_def_t {choice_var}[] = {{")
            for enum_index, val in enumerate(values):
                # Use raw mnemonic as choice string (no prefix), enum literal as tag
                enum_const = re.sub(r"[^0-9a-zA-Z]", "_", val).upper()
                enum_const_full = f"{param_name.upper()}_{enum_const}"
                lines.append(f"    {{\"{val}\", {enum_const_full}}},")
            lines.append(f"    SCPI_CHOICE_LIST_END")
            lines.append(f"}};")
            lines.append("")
    
    return lines


def generate_function_prototypes(docs):
    """Generate all function prototypes for header file.
    Returns list of code lines."""
    lines = []
    
    lines.append("/* ============================================================================")
    lines.append(" * User-implemented callback functions (weak - provide override)")
    lines.append(" * Return: SCPI_ERROR_NO_ERROR (0) or positive on success,")
    lines.append(" *         negative SCPI error code on failure")
    lines.append(" *         (e.g., SCPI_ERROR_SETTINGS_CONFLICT, SCPI_ERROR_EXECUTION_ERROR)")
    lines.append(" * ========================================================================== */\n")
    
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        params = cmd_entry.get("params", [])
        indices = cmd_entry.get("indices", [])
        has_query = cmd_entry.get("has_query", False)
        is_query_only = is_query(cmd)
        
        ident = safe_ident(cmd)
        has_indices = len(indices) > 0
        has_params = len(params) > 0
        
        if is_query_only:
            # Query-only
            if not params:
                continue
            lines.append(f"/* Query-only: {cmd} */")
            parts = []
            if indices:
                parts.append(f"const unsigned int indices[{len(indices)}]")
            for param in params:
                ctype = get_param_type_c_for_param(ident, param)
                pname = param.get("name", "")
                parts.append(f"{ctype} *{pname}")
            lines.append(f"int custom_{ident}({', '.join(parts)});")
        else:
            # Event handler
            lines.append(f"/* Event: {cmd} */")
            parts = []
            if has_indices:
                parts.append(f"const unsigned int indices[{len(indices)}]")
            if has_params:
                for param in params:
                    ctype = get_param_type_c_for_param(ident, param)
                    pname = param.get("name", "")
                    parts.append(f"{ctype} {pname}")
            if not parts:
                parts.append("void")
            lines.append(f"int custom_{ident}({', '.join(parts)});")
            
            # Query handler (if has_query)
            if has_query and has_params:
                lines.append(f"/* Query: {cmd}? */")
                parts = []
                if has_indices:
                    parts.append(f"const unsigned int indices[{len(indices)}]")
                for param in params:
                    ctype = get_param_type_c_for_param(ident, param)
                    pname = param.get("name", "")
                    parts.append(f"{ctype} *{pname}")
                lines.append(f"int custom_{ident}_QUERY({', '.join(parts)});")
        
        lines.append("")
    
    return lines


def gen_md(docs, out):
    """Generate Markdown documentation"""
    out.write("# SCPI Command Reference for PICO APG\n\n")
    out.write("| Command | Parameters | Description | Details | Default | Status |\n")
    out.write("|---|---|---|---|---|---|\n")
    
    for c in docs:
        cmd = c.get("command", "")
        desc = c.get("description", "")
        status = c.get("status", "")
        
                
        # Add query 
        if c.get("has_query"):
            cmd_display = f"`{cmd}`<br>`{cmd}?`"
        else:
            cmd_display = f"`{cmd}`"

        # Build command column with index info on new line if present
        indices = c.get("indices", [])
        if indices:
            idx_strs = [f"{idx['name']}={idx['range']}" for idx in indices]
            cmd_display += "<br>" + ', '.join(idx_strs)

        
        # Build parameters column with code formatting
        param_strs = []
        for p in c.get("params", []):
            ptype = p.get("type", "")
            pname = p.get("name", "")
            if ptype == "enum":
                param_strs.append("\\|".join(p.get("values", [])))
            elif ptype in ("bool", "boolean"):
                param_strs.append("<bool>")
            elif pname:
                param_strs.append(f"<{pname}>")
        params = "`" + ", ".join(param_strs) + "`" if param_strs else "-"
        
        # Build details column: base details + MIN/MAX for value types
        detail_parts = []
        base = c.get("details", "")
        if base:
            detail_parts.append(base.replace("; ", "<br>"))
        
        for p in c.get("params", []):
            if p.get("type") != "enum":
                ranges = []
                if p.get("min") is not None:
                    ranges.append(f"MIN={p['min']}")
                if p.get("max") is not None:
                    ranges.append(f"MAX={p['max']}")
                if ranges:
                    detail_parts.append(", ".join(ranges))
        
        details = "<br>".join(detail_parts) or "-"
        
        # Build defaults column
        defaults = "; ".join(str(p["default"]) for p in c.get("params", []) if "default" in p) or "-"
        
        out.write(f"| {cmd_display} | {params} | {desc} | {details} | {defaults} | {status} |\n")


def gen_header(docs, header_path, c_path):
    """Generate header with parameter structures and function prototypes"""
    guard = "SCPI_COMMANDS_GEN_H"
    lines = []
    
    lines.append("/* Auto-generated SCPI handler stubs and command table")
    lines.append(" * Generated from: scpi_commands.yaml")
    lines.append(" * DO NOT EDIT - regenerate with: python tools/generate_scpi_artifacts.py scpi_server/scpi_commands.yaml --force")
    lines.append(" */")
    lines.append("#ifndef %s" % guard)
    lines.append("#define %s\n" % guard)
    lines.append("#include <stdint.h>")
    lines.append("#include <stdbool.h>")
    lines.append("#include <string.h>")
    lines.append("#include \"scpi/scpi.h\"")
    lines.append("#include \"scpi_enums_gen.h\"\n")
    
    # Function prototypes
    lines.extend(generate_function_prototypes(docs))
    
    # Command table macro
    lines.append("/* ============================================================================")
    lines.append(" * Generated command table macro - include in scpi-def.c")
    lines.append(" * ========================================================================== */\n")
    lines.append("/* Forward declarations of handlers */")
    
    # Forward declare all handlers
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        has_query = cmd_entry.get("has_query", False)
        
        if not is_query(cmd):
            ident = safe_ident(cmd)
            lines.append(f"scpi_result_t handler_{ident}(scpi_t *context);")
            if has_query:
                lines.append(f"scpi_result_t handler_{ident}_QUERY(scpi_t *context);")
        else:
            # Query-only
            ident = safe_ident(cmd)
            lines.append(f"scpi_result_t handler_{ident}(scpi_t *context);")
    
    lines.append("")
    lines.append("/* Macro to include all generated commands in scpi_commands[] */")
    lines.append("#define SCPI_GENERATED_COMMANDS \\")
    
    # Build macro entries
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        has_query = cmd_entry.get("has_query", False)
        
        # Convert <n> to # for SCPI pattern matching
        pattern = re.sub(r'<[^>]+>', '#', cmd)
        
        if not is_query(cmd):
            ident = safe_ident(cmd)
            lines.append(f"    {{ .pattern = \"{pattern}\", .callback = handler_{ident} }}, \\")
            if has_query:
                lines.append(f"    {{ .pattern = \"{pattern}?\", .callback = handler_{ident}_QUERY }}, \\")
        else:
            # Query-only
            ident = safe_ident(cmd)
            lines.append(f"    {{ .pattern = \"{pattern}\", .callback = handler_{ident} }}, \\")
    
    lines.append("    /* End of generated commands */")
    
    lines.append("\n#endif /* %s */" % guard)
    
    with open(header_path, "w", newline="\n") as f:
        f.write("\n".join(lines))
    
    # Generate C implementation file
    gen_c_handlers(docs, c_path)


def gen_c_handlers(docs, c_path):
    """Generate C implementation with SCPI handler callbacks"""
    lines = []
    
    lines.append("/* Auto-generated SCPI handlers")
    lines.append(" * Generated from: scpi_commands.yaml")
    lines.append(" * DO NOT EDIT - regenerate with: python tools/generate_scpi_artifacts.py scpi_server/scpi_commands.yaml --force")
    lines.append(" */")
    lines.append("#include \"scpi_commands_gen.h\"")
    lines.append("#include \"scpi-def.h\"")
    lines.append("#include <stdio.h>")
    lines.append("#include <stdlib.h>")
    lines.append("#include <ctype.h>\n")

    lines.append("/* ============================================================================")
    lines.append(" * Generated helper functions")
    lines.append(" * ========================================================================== */\n")

    lines.append("static inline scpi_result_t scpi_gen_parse_indices(scpi_t *context, unsigned int *indices, const unsigned int *min_vals, const unsigned int *max_vals, size_t count) {")
    lines.append("    if (!SCPI_CommandNumbers(context, (int32_t*)indices, count, -1)) {")
    lines.append("        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    for (size_t i = 0; i < count; ++i) {")
    lines.append("        if (indices[i] < min_vals[i] || indices[i] > max_vals[i]) {")
    lines.append("            SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);")
    lines.append("            return SCPI_RES_ERR;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return SCPI_RES_OK;")
    lines.append("}")
    lines.append("")

    lines.append("static inline scpi_result_t scpi_gen_parse_int(scpi_t *context, int *out, bool has_min, int min_val, bool has_max, int max_val) {")
    lines.append("    int32_t value;")
    lines.append("    if (!SCPI_ParamInt32(context, &value, TRUE)) {")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    if ((has_min && value < min_val) || (has_max && value > max_val)) {")
    lines.append("        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    *out = (int)value;")
    lines.append("    return SCPI_RES_OK;")
    lines.append("}")
    lines.append("")

    lines.append("static inline scpi_result_t scpi_gen_parse_uint(scpi_t *context, unsigned int *out, bool has_min, unsigned int min_val, bool has_max, unsigned int max_val) {")
    lines.append("    uint32_t value;")
    lines.append("    if (!SCPI_ParamUInt32(context, &value, TRUE)) {")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    if ((has_min && value < min_val) || (has_max && value > max_val)) {")
    lines.append("        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    *out = (unsigned int)value;")
    lines.append("    return SCPI_RES_OK;")
    lines.append("}")
    lines.append("")

    lines.append("static inline scpi_result_t scpi_gen_parse_float(scpi_t *context, float *out, bool has_min, float min_val, bool has_max, float max_val) {")
    lines.append("    float value;")
    lines.append("    if (!SCPI_ParamFloat(context, &value, TRUE)) {")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    if ((has_min && value < min_val) || (has_max && value > max_val)) {")
    lines.append("        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    *out = value;")
    lines.append("    return SCPI_RES_OK;")
    lines.append("}")
    lines.append("")

    lines.append("static inline scpi_result_t scpi_gen_parse_bool(scpi_t *context, bool *out) {")
    lines.append("    scpi_bool_t value;")
    lines.append("    if (!SCPI_ParamBool(context, &value, TRUE)) {")
    lines.append("        return SCPI_RES_ERR;")
    lines.append("    }")
    lines.append("    *out = value ? true : false;")
    lines.append("    return SCPI_RES_OK;")
    lines.append("}")
    lines.append("")



    
    # Generate choice lists for enum parameters (one per command with enum)
    lines.append("/* ============================================================================")
    lines.append(" * Choice lists for enum parameters")
    lines.append(" * ========================================================================== */\n")
    
    generated_choices = set()  # Track which choice list names we've generated
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        if is_query(cmd):
            continue
        params = cmd_entry.get("params", [])
        for param in params:
            if param.get("type") == "enum":
                param_name = param.get("name", "")
                choice_var = f"source_{safe_ident(param_name)}_choices"
                # Only generate this choice list once
                if choice_var not in generated_choices:
                    values = param.get("values", [])
                    lines.append(f"static const scpi_choice_def_t {choice_var}[] = {{")
                    for enum_index, val in enumerate(values):
                        enum_const = re.sub(r"[^0-9a-zA-Z]", "_", val).upper()
                        enum_const_full = f"{param_name.upper()}_{enum_const}"
                        lines.append(f"    {{\"{val}\", {enum_const_full}}},")
                    lines.append(f"    SCPI_CHOICE_LIST_END")
                    lines.append(f"}};")
                    lines.append("")
                    generated_choices.add(choice_var)
    
    # Stub implementations for planned (unimplemented) commands
    # - status: "planned" => Generate non-weak stub (linker error if not overridden)
    # - No status field => No stub (implementation must exist in scpi_commands.c)
    lines.append("/* ============================================================================")
    lines.append(" * Stub implementations for planned (unimplemented) commands")
    lines.append(" * Only stubs for commands with status: \"planned\" are generated.")
    lines.append(" * Commands without status are assumed to be implemented in scpi_commands.c")
    lines.append(" * ========================================================================== */\n")
    
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        status = cmd_entry.get("status")
        
        # Skip if no status field (command is implemented)
        if status is None:
            continue
        
        # Only generate stubs for planned commands
        if status != "planned":
            continue
        
        params = cmd_entry.get("params", [])
        indices = cmd_entry.get("indices", [])
        has_query = cmd_entry.get("has_query", False)
        is_query_only = is_query(cmd)
        
        if is_query_only:
            if not params:
                continue  # Skip query-only without response
            ident = safe_ident(cmd)
            parts = []
            if indices:
                parts.append(f"const unsigned int indices[{len(indices)}]")
            for param in params:
                ctype = get_param_type_c_for_param(ident, param)
                pname = param.get("name", "")
                parts.append(f"{ctype} *{pname}")
            
            # Non-weak stub for planned (unimplemented) command
            lines.append(f"int custom_{ident}({', '.join(parts)}) {{")
            if indices:
                lines.append(f"    (void)indices;")
            for param in params:
                pname = param.get("name", "")
                lines.append(f"    (void){pname};")
            lines.append(f"    return SCPI_ERROR_EXECUTION_ERROR; /* Not implemented */")
            lines.append(f"}}\n")
        else:
            ident = safe_ident(cmd)
            has_indices = len(indices) > 0
            has_params = len(params) > 0
            
            # Event stub
            parts = []
            if has_indices:
                parts.append(f"const unsigned int indices[{len(indices)}]")
            if has_params:
                for param in params:
                    ctype = get_param_type_c_for_param(ident, param)
                    pname = param.get("name", "")
                    parts.append(f"{ctype} {pname}")
            if not parts:
                parts.append("void")
            
            # Non-weak stub for planned (unimplemented) command
            lines.append(f"int custom_{ident}({', '.join(parts)}) {{")
            if has_indices:
                lines.append(f"    (void)indices;")
            if has_params:
                for param in params:
                    pname = param.get("name", "")
                    lines.append(f"    (void){pname};")
            lines.append(f"    return SCPI_ERROR_EXECUTION_ERROR; /* Not implemented */")
            lines.append(f"}}\n")
            
            # Query stub (if has_query)
            if has_query and has_params:
                parts = []
                if has_indices:
                    parts.append(f"const unsigned int indices[{len(indices)}]")
                for param in params:
                    ctype = get_param_type_c_for_param(ident, param)
                    pname = param.get("name", "")
                    parts.append(f"{ctype} *{pname}")
                
                # Non-weak stub for planned (unimplemented) command
                lines.append(f"int custom_{ident}_QUERY({', '.join(parts)}) {{")
                if has_indices:
                    lines.append(f"    (void)indices;")
                for param in params:
                    pname = param.get("name", "")
                    lines.append(f"    (void){pname};")
                lines.append(f"    return SCPI_ERROR_EXECUTION_ERROR; /* Not implemented */")
                lines.append(f"}}\n")
    
    # SCPI handler implementations
    lines.append("/* ============================================================================")
    lines.append(" * SCPI Command Handlers (parse, validate, call custom hooks)")
    lines.append(" * ========================================================================== */\n")
    
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        params = cmd_entry.get("params", [])
        indices = cmd_entry.get("indices", [])
        has_query = cmd_entry.get("has_query", False)
        is_query_only = is_query(cmd)
        
        if is_query_only and not has_query:
            continue  # Skip query-only without response
        
        ident = safe_ident(cmd)
        has_indices = len(indices) > 0
        has_params = len(params) > 0
        
        # For non-query commands: generate event handler and optional query handler
        # For query-only commands: generate single handler
        handlers_to_gen = []
        
        if not is_query_only:
            handlers_to_gen.append(("event", False))
            if has_query and has_params:
                handlers_to_gen.append(("query", True))
        else:
            # Query-only
            if has_params:
                handlers_to_gen.append(("query", True))
        
        for handler_type, is_response_query in handlers_to_gen:
            # Handler signature
            if handler_type == "event":
                lines.append(f"scpi_result_t handler_{ident}(scpi_t *context) {{")
            else:
                suffix = "_QUERY" if not is_query_only else ""
                lines.append(f"scpi_result_t handler_{ident}{suffix}(scpi_t *context) {{")
            
            # Declare variables
            if has_params:
                for param in params:
                    ctype = get_param_type_c_for_param(ident, param)
                    pname = param.get("name", "")
                    lines.append(f"    {ctype} {pname} = 0;")
            if has_indices:
                lines.append(f"    unsigned int indices[{len(indices)}] = {{0}};")
            lines.append(f"    int result;")
            lines.append("")
            
            # Extract and validate indices
            lines.extend(generate_index_extraction_code(indices))
            
            # Parse and validate parameters (only for EVENT handlers, not QUERY)
            if handler_type == "event":
                lines.extend(generate_param_parsing_code(ident, params))
            
            # Call custom hook
            if handler_type == "event":
                lines.append(f"    /* Call custom implementation */")
            else:
                lines.append(f"    /* Call custom query implementation */")
            
            args = []
            if has_indices:
                args.append("indices")
            if has_params:
                for param in params:
                    if handler_type == "event":
                        args.append(param.get("name", ""))
                    else:
                        args.append(f"&{param.get('name', '')}")
            
            if handler_type == "event":
                custom_func = f"custom_{ident}"
            else:
                custom_func = f"custom_{ident}_QUERY" if not is_query_only else f"custom_{ident}"
            
            if args:
                lines.append(f"    result = {custom_func}({', '.join(args)});")
            else:
                lines.append(f"    result = {custom_func}();")
            
            lines.append(f"    if (result < 0) {{")
            lines.append(f"        SCPI_ErrorPush(context, result);")
            lines.append(f"        return SCPI_RES_ERR;")
            lines.append(f"    }}")
            lines.append("")
            
            # Format response if query
            if handler_type == "query":
                lines.extend(generate_response_formatting_code(ident, params))
            
            lines.append(f"    return SCPI_RES_OK;")
            lines.append(f"}}\n")
    
    with open(c_path, "w", newline="\n") as f:
        f.write("\n".join(lines))



def should_regenerate(input_path, output_paths, force=False):
    """Check if output files need regeneration based on input modification time.
    
    Args:
        input_path: Path to input YAML file
        output_paths: List of output file paths to check
        force: If True, always regenerate
    
    Returns:
        True if any output is missing or older than input
    """
    if force:
        return True
    
    if not os.path.exists(input_path):
        raise FileNotFoundError(f"Input file not found: {input_path}")
    
    input_mtime = os.path.getmtime(input_path)
    
    for output_path in output_paths:
        if not os.path.exists(output_path):
            return True
        if os.path.getmtime(output_path) < input_mtime:
            return True
    
    return False



def build_enum_type_mapping(docs):
    """Build the enum type mapping for deduplication.
    This should be called first, before generating any code."""
    global _ENUM_TYPE_MAPPING
    _ENUM_TYPE_MAPPING.clear()
    
    enum_value_map = {}  # tuple(values) -> first_enum_type_name
    
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        params = cmd_entry.get("params", [])
        
        if not params or is_query(cmd):
            continue
        
        ident = safe_ident(cmd)
        
        for param in params:
            if param.get("type") == "enum":
                param_name = param.get("name", "")
                enum_name = get_enum_type_name(ident, param_name)
                values = param.get("values", [])
                value_tuple = tuple(values)
                
                # If this value combination already exists, reuse the first enum type name
                if value_tuple in enum_value_map:
                    first_enum_name = enum_value_map[value_tuple]
                    _ENUM_TYPE_MAPPING[enum_name] = first_enum_name
                else:
                    # First occurrence of this value combination
                    enum_value_map[value_tuple] = enum_name


def gen_enums(docs, enums_path):
    """Generate standalone scpi_enums.h with all enum definitions.
    
    This header is independent of scpi_commands_gen.h and can be safely included
    by PWM subsystem and other modules without pulling in full SCPI machinery.
    
    Uses pre-built _ENUM_TYPE_MAPPING (call build_enum_type_mapping first).
    """
    lines = []
    
    lines.append("/*")
    lines.append(" * Copyright (c) 2026 honsma235")
    lines.append(" * SPDX-License-Identifier: GPL-2.0-only")
    lines.append(" *")
    lines.append(" * SCPI Enumerations (Auto-Generated)")
    lines.append(" *")
    lines.append(" * This header contains all generated enums from scpi_commands.yaml.")
    lines.append(" * It is independent of scpi_commands_gen.h and can be safely included")
    lines.append(" * by PWM subsystem and other modules without pulling in full SCPI machinery.")
    lines.append(" *")
    lines.append(" * Auto-generated. Do not edit directly - regenerate via:")
    lines.append(" *   python tools/generate_scpi_artifacts.py scpi_server/scpi_commands.yaml --force")
    lines.append(" */")
    lines.append("")
    lines.append("#ifndef SCPI_ENUMS_GEN_H")
    lines.append("#define SCPI_ENUMS_GEN_H")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append("extern \"C\" {")
    lines.append("#endif")
    lines.append("")
    
    # Track enum values to avoid duplicate typedefs
    # Maps tuple of enum values -> (first_enum_type_name, enum_definition_lines)
    enum_value_map = {}  # tuple(values) -> (enum_type_name, lines)
    processed_enums = {}  # Maps enum type name to enum definition lines
    
    # Collect all enums (using pre-built deduplication mapping)
    for cmd_entry in docs:
        cmd = cmd_entry.get("command", "")
        params = cmd_entry.get("params", [])
        
        if not params or is_query(cmd):
            continue
        
        ident = safe_ident(cmd)
        
        for param in params:
            if param.get("type") == "enum":
                param_name = param.get("name", "")
                enum_name = get_enum_type_name(ident, param_name)
                values = param.get("values", [])
                value_tuple = tuple(values)
                
                # If already processed (or mapped), skip
                if enum_name in processed_enums:
                    continue
                if enum_name in _ENUM_TYPE_MAPPING:
                    # This enum was deduplicated - it will be generated under its canonical name
                    continue
                
                # If this value combination already exists, it was tracked during mapping
                if value_tuple in enum_value_map:
                    continue
                
                # First occurrence of this value combination - generate the typedef
                enum_lines = []
                enum_lines.append(f"/* {param_name}: {', '.join(values)} */")
                enum_lines.append(f"typedef enum {{")
                for enum_index, val in enumerate(values):
                    enum_val = re.sub(r"[^0-9a-zA-Z]", "_", val).upper()
                    enum_val_prefixed = f"{param_name.upper()}_{enum_val}"
                    enum_lines.append(f"    {enum_val_prefixed} = {enum_index},")
                enum_lines.append(f"}} {enum_name};")
                enum_lines.append("")
                
                enum_value_map[value_tuple] = (enum_name, enum_lines)
                processed_enums[enum_name] = enum_lines
    
    # Write all enums in order
    for enum_lines in processed_enums.values():
        lines.extend(enum_lines)
    
    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif /* SCPI_ENUMS_GEN_H */")
    
    with open(enums_path, "w", newline="\n") as f:
        f.write("\n".join(lines))


def main():

    ap = argparse.ArgumentParser(description="Generate SCPI documentation and C code artifacts from YAML")
    ap.add_argument("yaml", help="Path to scpi_commands.yaml")
    ap.add_argument("--force", action="store_true", help="Regenerate even if outputs are newer than input")
    args = ap.parse_args()

    docs = yaml.safe_load(open(args.yaml)) or []
    yaml_dir = os.path.dirname(args.yaml) or "."

    # Output paths derived from input
    md_path = os.path.join(yaml_dir, "SCPI_COMMANDS.md")
    header_path = os.path.join(yaml_dir, "scpi_commands_gen.h")
    c_path = os.path.join(yaml_dir, "scpi_commands_gen.c")
    enums_path = os.path.join(yaml_dir, "scpi_enums_gen.h")

    # Build enum mapping FIRST before any code generation
    build_enum_type_mapping(docs)

    generated = []

    # Generate markdown
    if should_regenerate(args.yaml, [md_path], args.force):
        with open(md_path, 'w') as f:
            gen_md(docs, f)
        generated.append(md_path)
    
    # Generate header and C files (uses the pre-built mapping)
    if should_regenerate(args.yaml, [header_path, c_path], args.force):
        gen_header(docs, header_path, c_path)
        generated.append(header_path)
        generated.append(c_path)

    # Generate enums header (also uses the pre-built mapping)
    if should_regenerate(args.yaml, [enums_path], args.force):
        gen_enums(docs, enums_path)
        generated.append(enums_path)

    # Report generated files
    if generated:
        for path in generated:
            print(f"Generated: {path}")
    else:
        print("Output files are up-to-date (use --force to regenerate)")


if __name__ == '__main__':
    main()
