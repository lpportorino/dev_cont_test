#!/bin/bash
# Jettison OSD Package Validation Script
# Validates package integrity: JWT signature, checksums, structure

set -u

# ============================================================================
# Configuration
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
KEYS_DIR="$PROJECT_ROOT/keys"
PUBLIC_KEY="$KEYS_DIR/example-public.pem"

# ============================================================================
# Helper Functions
# ============================================================================

log() {
    echo "[VALIDATE] $*"
}

error() {
    echo "[ERROR] $*" >&2
    exit 1
}

success() {
    echo "[✅] $*"
}

# Base64url decode
base64url_decode() {
    tr '_-' '/+' | base64 -d
}

# ============================================================================
# Validation Functions
# ============================================================================

validate_jwt_signature() {
    local jwt_file="$1"

    log "Validating JWT signature..."

    # Split JWT into parts
    local jwt=$(cat "$jwt_file")
    local header_b64=$(echo "$jwt" | cut -d. -f1)
    local payload_b64=$(echo "$jwt" | cut -d. -f2)
    local signature_b64=$(echo "$jwt" | cut -d. -f3)

    # Reconstruct signing input
    local signing_input="${header_b64}.${payload_b64}"

    # Decode signature from base64url to binary file
    local temp_sig=$(mktemp)
    echo -n "$signature_b64" | base64url_decode > "$temp_sig"

    # Verify signature
    if echo -n "$signing_input" | openssl dgst -sha256 -verify "$PUBLIC_KEY" -signature "$temp_sig" &>/dev/null; then
        rm -f "$temp_sig"
        success "JWT signature valid"
        return 0
    else
        rm -f "$temp_sig"
        error "JWT signature verification FAILED"
    fi
}

decode_jwt_payload() {
    local jwt_file="$1"
    local jwt=$(cat "$jwt_file")
    local payload_b64=$(echo "$jwt" | cut -d. -f2)
    echo "$payload_b64" | base64url_decode
}

validate_checksums() {
    local work_dir="$1"
    local manifest_file="$2"

    log "Validating checksums..."

    # Validate inner archive checksum
    local archive_file=$(jq -r '.archive.filename' "$manifest_file")
    local expected_sha=$(jq -r '.archive.sha256' "$manifest_file")
    local actual_sha=$(sha256sum "$work_dir/$archive_file" | awk '{print $1}')

    if [[ "$expected_sha" != "$actual_sha" ]]; then
        error "Archive checksum mismatch: expected $expected_sha, got $actual_sha"
    fi
    success "Archive checksum valid: $archive_file"

    # Extract inner archive
    tar -xzf "$work_dir/$archive_file" -C "$work_dir"

    # Validate WASM binary checksum
    local wasm_file=$(jq -r '.contents.wasm.filename' "$manifest_file")
    local expected_wasm_sha=$(jq -r '.contents.wasm.sha256' "$manifest_file")
    local actual_wasm_sha=$(sha256sum "$work_dir/$wasm_file" | awk '{print $1}')

    if [[ "$expected_wasm_sha" != "$actual_wasm_sha" ]]; then
        error "WASM checksum mismatch: expected $expected_wasm_sha, got $actual_wasm_sha"
    fi
    success "WASM binary checksum valid: $wasm_file"

    # Validate config checksum
    local config_file=$(jq -r '.contents.config.filename' "$manifest_file")
    local expected_config_sha=$(jq -r '.contents.config.sha256' "$manifest_file")
    local actual_config_sha=$(sha256sum "$work_dir/$config_file" | awk '{print $1}')

    if [[ "$expected_config_sha" != "$actual_config_sha" ]]; then
        error "Config checksum mismatch: expected $expected_config_sha, got $actual_config_sha"
    fi
    success "Config checksum valid: $config_file"

    # Validate resource checksums
    local resource_count=$(jq '.contents.resources | length' "$manifest_file")
    local valid_resources=0

    for ((i=0; i<$resource_count; i++)); do
        local resource_path=$(jq -r ".contents.resources[$i].path" "$manifest_file")
        local expected_resource_sha=$(jq -r ".contents.resources[$i].sha256" "$manifest_file")

        if [[ ! -f "$work_dir/$resource_path" ]]; then
            error "Resource file missing: $resource_path"
        fi

        local actual_resource_sha=$(sha256sum "$work_dir/$resource_path" | awk '{print $1}')

        if [[ "$expected_resource_sha" != "$actual_resource_sha" ]]; then
            error "Resource checksum mismatch: $resource_path"
        fi

        ((valid_resources++))
    done

    success "All $valid_resources resource checksums valid"
}

validate_manifest_schema() {
    local manifest_file="$1"

    log "Validating manifest structure..."

    # Check required fields
    local required_fields=(
        ".manifest_version"
        ".package.name"
        ".package.version"
        ".package.variant"
        ".package.created_at"
        ".package.git_sha"
        ".package.build_mode"
        ".archive.filename"
        ".archive.sha256"
        ".archive.size_bytes"
        ".contents.wasm.filename"
        ".contents.wasm.sha256"
        ".contents.config.filename"
        ".contents.config.sha256"
        ".system_requirements.wasm_runtime"
        ".system_requirements.resolution"
        ".system_requirements.mode"
        ".system_requirements.stream_type"
        ".signature.algorithm"
        ".signature.public_key_fingerprint"
    )

    for field in "${required_fields[@]}"; do
        if ! jq -e "$field" "$manifest_file" &>/dev/null; then
            error "Missing required field: $field"
        fi
    done

    success "Manifest structure valid"
}

display_package_info() {
    local manifest_file="$1"

    echo ""
    echo "=========================================="
    echo "  Package Information"
    echo "=========================================="
    echo ""

    jq -r '"Name:        " + .package.name' "$manifest_file"
    jq -r '"Version:     " + .package.version' "$manifest_file"
    jq -r '"Variant:     " + .package.variant' "$manifest_file"
    jq -r '"Created:     " + .package.created_at' "$manifest_file"
    jq -r '"Git SHA:     " + .package.git_sha' "$manifest_file"
    jq -r '"Build mode:  " + .package.build_mode' "$manifest_file"
    echo ""
    jq -r '"Resolution:  " + .system_requirements.resolution' "$manifest_file"
    jq -r '"Mode:        " + .system_requirements.mode' "$manifest_file"
    jq -r '"Stream:      " + .system_requirements.stream_type' "$manifest_file"
    jq -r '"WASM size:   " + (.contents.wasm.size_bytes | tostring) + " bytes"' "$manifest_file"

    echo ""
}

# ============================================================================
# Main Validation Function
# ============================================================================

validate_package() {
    local package_file="$1"

    if [[ ! -f "$package_file" ]]; then
        echo "[ERROR] Package file not found: $package_file" >&2
        return 1
    fi

    log "Validating package: $(basename "$package_file")"

    # Create temporary working directory
    local work_dir=$(mktemp -d)

    # Extract outer archive
    log "Extracting package..."
    if ! tar -xf "$package_file" -C "$work_dir" 2>/dev/null; then
        rm -rf "$work_dir"
        echo "[ERROR] Failed to extract package" >&2
        return 1
    fi

    # Check for JWT
    if [[ ! -f "$work_dir/manifest.jwt" ]]; then
        rm -rf "$work_dir"
        echo "[ERROR] manifest.jwt not found in package" >&2
        return 1
    fi

    # Validate JWT signature
    if ! validate_jwt_signature "$work_dir/manifest.jwt" 2>/dev/null; then
        rm -rf "$work_dir"
        return 1
    fi

    # Decode JWT payload to manifest.json
    local manifest_file="$work_dir/manifest.json"
    if ! decode_jwt_payload "$work_dir/manifest.jwt" > "$manifest_file" 2>/dev/null; then
        rm -rf "$work_dir"
        echo "[ERROR] Failed to decode JWT payload" >&2
        return 1
    fi

    # Validate manifest structure
    if ! validate_manifest_schema "$manifest_file" 2>/dev/null; then
        rm -rf "$work_dir"
        return 1
    fi

    # Validate checksums
    if ! validate_checksums "$work_dir" "$manifest_file" 2>/dev/null; then
        rm -rf "$work_dir"
        return 1
    fi

    # Display package info
    display_package_info "$manifest_file"

    # Cleanup
    rm -rf "$work_dir"

    echo "=========================================="
    success "Package validation PASSED"
    echo "=========================================="
    echo ""

    return 0
}

# ============================================================================
# Main Entry Point
# ============================================================================

main() {
    if [[ $# -eq 0 ]]; then
        echo "Usage: $0 <package.tar> [package2.tar ...]"
        echo ""
        echo "Validates Jettison OSD package integrity:"
        echo "  - JWT signature verification"
        echo "  - Manifest structure validation"
        echo "  - SHA256 checksum verification"
        echo ""
        echo "Examples:"
        echo "  $0 dist/jettison-osd-live_day-1.0.0.tar"
        echo "  $0 dist/*.tar"
        exit 1
    fi

    # Check for required tools
    for cmd in openssl jq tar sha256sum; do
        if ! command -v $cmd &>/dev/null; then
            error "Required command not found: $cmd"
        fi
    done

    # Check for public key
    if [[ ! -f "$PUBLIC_KEY" ]]; then
        error "Public key not found: $PUBLIC_KEY"
    fi

    # Validate each package
    local total=$#
    local passed=0
    local failed=0

    for package in "$@"; do
        if validate_package "$package"; then
            ((passed++))
        else
            ((failed++))
        fi
    done

    echo ""
    echo "=========================================="
    echo "  Validation Summary"
    echo "=========================================="
    echo "Total:  $total packages"
    echo "Passed: $passed"
    echo "Failed: $failed"
    echo "=========================================="
    echo ""

    if [[ $failed -gt 0 ]]; then
        exit 1
    fi
}

main "$@"
