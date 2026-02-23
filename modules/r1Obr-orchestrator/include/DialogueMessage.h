/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <yarp/os/LogStream.h>


using json = nlohmann::json;

namespace dlgmsg {
/**
 * @enum CmdTypes
 * @brief Enumeration of supported dialogue command types
 *
 * This enum defines all possible command types that can be issued through
 * the dialogue system. Each type corresponds to a specific action or query
 * that the dialogue system should execute.
 */
enum CmdTypes
{
    GO,
    SEARCH,
    STOP,
    RESUME,
    RESET,
    SAY,
    WHERE,
    WHAT,
    STATUS,
    NAVPOS,
    GUIDE,
    DIRECTIONS,
    SUCCESS,
    FAILED,
    FAREWELL,
    IGNORE,
    GRASP,
    INVALID = -1
};

/**
 * @brief JSON serialization mapping for CmdTypes enum
 *
 * Maps CmdTypes enum values to their string representations for JSON serialization/deserialization.
 * This allows CmdTypes enums to be automatically converted to/from JSON strings.
 */
// map TaskState values to JSON as strings
NLOHMANN_JSON_SERIALIZE_ENUM(CmdTypes, {{INVALID, "invalid"},
                                        {GO, "go"},
                                        {SEARCH, "search"},
                                        {STOP, "stop"},
                                        {RESUME, "resume"},
                                        {RESET, "reset"},
                                        {WHERE, "where"},
                                        {WHAT, "what"},
                                        {STATUS, "status"},
                                        {NAVPOS, "navpos"},
                                        {GUIDE, "guide"},
                                        {DIRECTIONS, "directions"},
                                        {SUCCESS, "success"},
                                        {FAILED, "failed"},
                                        {FAREWELL, "farewell"},
                                        {IGNORE, "ignore"},
                                        {SAY, "say"},
                                        {GRASP, "grasping"}})

/**
 * @class DialogueMessage
 * @brief A class representing a dialogue message with command type, parameters, language, and metadata.
 *
 * This class encapsulates dialogue commands that can be serialized to/from JSON format.
 * It contains information about the command type, its parameters, the language used,
 * and optional query and comment fields.
 */
class DialogueMessage
{
protected:
    /// @brief The command type (e.g., GO, SEARCH, STOP, etc.)
    CmdTypes m_type{INVALID};

    /// @brief Vector of parameters associated with the command
    std::vector<std::string> m_params;

    /// @brief The language in which the dialogue is conducted
    std::string m_language;

    /// @brief Optional query string associated with the command
    std::string m_query;

    /// @brief Optional comment or additional information about the command
    std::string m_comment;

public:
    //NLOHMANN_DEFINE_TYPE_INTRUSIVE(DialogueMessage, m_type, m_params, m_language, m_query, m_comment)

    /**
     * @brief Default constructor creating an invalid DialogueMessage
     *
     * Initializes all member variables to their default values.
     * The m_type is initialized to INVALID.
     */
    DialogueMessage() = default;

    /**
     * @brief Full constructor with all parameters
     *
     * Initializes a DialogueMessage with the specified command type, parameters, language,
     * and optional query and comment fields.
     *
     * @param type The command type for this message
     * @param params Vector of string parameters associated with the command
     * @param language The language in which the dialogue is conducted
     * @param query Optional query string (default: empty string)
     * @param comment Optional comment string (default: empty string)
     */
    DialogueMessage(CmdTypes type, const std::vector<std::string>& params, const std::string& language, const std::string& query="", const std::string& comment="");

    /**
     * @brief Move constructor
     * @param movingCommand the command to move
     */
    DialogueMessage(DialogueMessage&& movingCommand) noexcept = default;

    /**
     * @brief Copy constructor
     * @param copiedCommand the command to copy
     */
    DialogueMessage(DialogueMessage& copiedCommand) noexcept = default;

    /**
     * @brief Assignment operator
     *
     * Copies all member variables from the source DialogueMessage object.
     * Returns a reference to this object to allow chaining.
     *
     * @param anotherCommand The DialogueMessage object to copy from
     * @return Reference to this DialogueMessage object
     */
    DialogueMessage& operator=(const DialogueMessage& anotherCommand);

    /**
     * @brief Get the command type
     *
     * @return The command type (CmdTypes enum value)
     */
    [[nodiscard]] CmdTypes getType() const;

    /**
     * @brief Convert command type to string representation
     *
     * Maps the numeric command type to its string representation for debugging
     * and logging purposes. Uses a static map for efficient lookup.
     *
     * @return String representation of the command type (e.g., "go", "search")
     * @throws std::out_of_range if the command type is not found in the map
     */
    [[nodiscard]] std::string getTypeAsString() const;

    /**
     * @brief Get the command parameters
     *
     * @return Const reference to the vector of string parameters
     */
    [[nodiscard]] const std::vector<std::string>& getParams() const;

    /**
     * @brief Get the language of the dialogue
     *
     * @return Const reference to the language string
     */
    [[nodiscard]] const std::string& getLanguage() const;

    /**
     * @brief Get the optional query string
     *
     * @return Const reference to the query string
     */
    [[nodiscard]] const std::string& getQuery() const;

    /**
     * @brief Get the optional comment string
     *
     * @return Const reference to the comment string
     */
    [[nodiscard]] const std::string& getComment() const;

    /**
     * @brief Set the command type
     *
     * Updates the command type to the specified value.
     *
     * @param type The command type to set
     * @return True (operation always succeeds)
     * @note TODO: Decide if the return value is useful
     */
    bool setType(CmdTypes type);

    /**
     * @brief Set the command parameters
     *
     * Updates the parameters vector with the specified values.
     *
     * @param params Vector of string parameters to set
     * @return True (operation always succeeds)
     * @note TODO: Decide if the return value is useful
     */
    bool setParams(const std::vector<std::string>& params);

    /**
     * @brief Set the dialogue language
     *
     * Updates the language for the dialogue.
     *
     * @param language The language string to set
     * @return True (operation always succeeds)
     * @note TODO: Decide if the return value is useful
     */
    bool setLanguage(const std::string& language);

    /**
     * @brief Set the optional query string
     *
     * Updates the query field associated with the command.
     *
     * @param query The query string to set
     * @return True (operation always succeeds)
     * @note TODO: Decide if the return value is useful
     */
    bool setQuery(const std::string& query);

    /**
     * @brief Set the optional comment string
     *
     * Updates the comment field associated with the command.
     *
     * @param comment The comment string to set
     * @return True (operation always succeeds)
     * @note TODO: Decide if the return value is useful
     */
    bool setComment(const std::string& comment);

};

/**
 * @brief Deserialize a DialogueMessage from JSON format
 *
 * Converts a JSON object into a DialogueMessage instance. The m_type and m_language
 * fields are required, while m_params is always expected. The m_query and m_comment
 * fields are optional.
 *
 * @param j The JSON object containing the serialized DialogueMessage
 * @param msg The DialogueMessage object to populate with deserialized data
 *
 * @throws nlohmann::json::exception if required fields (m_type, m_params, m_language) are missing
 */
inline void from_json(const json& j, DialogueMessage& msg) {
    CmdTypes type;
    j.at("m_type").get_to(type);
    msg.setType(type);

    std::vector<std::string> params;
    j.at("m_params").get_to(params);
    msg.setParams(params);

    std::string language;
    j.at("m_language").get_to(language);
    msg.setLanguage(language);

    if (j.contains("m_query")) {
        std::string query;
        j.at("m_query").get_to(query);
        msg.setQuery(query);
    }
    if (j.contains("m_comment")) {
        std::string comment;
        j.at("m_comment").get_to(comment);
        msg.setComment(comment);
    }
};

/**
 * @brief Serialize a DialogueMessage to JSON format
 *
 * Converts a DialogueMessage instance into a JSON object. The m_type, m_params,
 * and m_language fields are always included. The m_query and m_comment fields
 * are only included in the JSON if they are not empty strings.
 *
 * @param j The JSON object to populate with serialized DialogueMessage data
 * @param msg The DialogueMessage object to serialize
 */
inline void to_json(json& j, const DialogueMessage& msg) {
    j = json{
        {"m_type", msg.getType()},
        {"m_params", msg.getParams()},
        {"m_language", msg.getLanguage()}
    };

    // Conditionally add m_query if not empty
    if (!msg.getQuery().empty()) {
        j["m_query"] = msg.getQuery();
    }

    // Conditionally add m_comment if not empty
    if (!msg.getComment().empty()) {
        j["m_comment"] = msg.getComment();
    }
};

}

