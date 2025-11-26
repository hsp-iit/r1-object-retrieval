#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CORRECT_PATH="$SCRIPT_DIR"
BASHRC="$HOME/.bashrc"
VAR_NAME="R1_OBJECT_RETRIEVAL_SOURCE_DIR"

# Function to add export to .bashrc
add_to_bashrc() {
    echo "" >> "$BASHRC"
    echo "# Added by r1-object-retrieval fixEnv.sh" >> "$BASHRC"
    echo "export $VAR_NAME=\"$CORRECT_PATH\"" >> "$BASHRC"
    echo "Added export to $BASHRC"
}

# Function to update existing export in .bashrc and sourced files
update_in_bashrc() {
    local old_value="$1"

    # Check .bashrc itself
    if grep -q "export $VAR_NAME=" "$BASHRC"; then
        sed -i "s|export $VAR_NAME=.*|export $VAR_NAME=\"$CORRECT_PATH\"|g" "$BASHRC"
        echo "Updated $VAR_NAME in $BASHRC"
        return 0
    fi

    # Check files sourced by .bashrc
    local sourced_files=$(grep -E "^\s*(source|\.) " "$BASHRC" | sed -E 's/^\s*(source|\.)\s+//' | sed 's/[\"'\'']//g')
    for file in $sourced_files; do
        # Expand ~ and variables
        file=$(eval echo "$file")
        if [ -f "$file" ] && grep -q "export $VAR_NAME=" "$file"; then
            sed -i "s|export $VAR_NAME=.*|export $VAR_NAME=\"$CORRECT_PATH\"|g" "$file"
            echo "Updated $VAR_NAME in $file"
            return 0
        fi
    done

    # If not found in sourced files, add to .bashrc
    add_to_bashrc
}

# Main logic
if [ -z "${R1_OBJECT_RETRIEVAL_SOURCE_DIR}" ]; then
    echo "$VAR_NAME is not set. Adding to $BASHRC..."
    add_to_bashrc
    export R1_OBJECT_RETRIEVAL_SOURCE_DIR="$CORRECT_PATH"
elif [ "${R1_OBJECT_RETRIEVAL_SOURCE_DIR}" != "$CORRECT_PATH" ]; then
    echo "Warning: $VAR_NAME is set to wrong value: ${R1_OBJECT_RETRIEVAL_SOURCE_DIR}"
    echo "Correct value should be: $CORRECT_PATH"
    echo "Updating in $BASHRC and sourced files..."
    update_in_bashrc "${R1_OBJECT_RETRIEVAL_SOURCE_DIR}"
    export R1_OBJECT_RETRIEVAL_SOURCE_DIR="$CORRECT_PATH"
else
    echo "$VAR_NAME is already correctly set to: ${R1_OBJECT_RETRIEVAL_SOURCE_DIR}"
fi

echo "Current session now has: $VAR_NAME=$R1_OBJECT_RETRIEVAL_SOURCE_DIR"
echo "Please restart your shell or run: source $BASHRC"