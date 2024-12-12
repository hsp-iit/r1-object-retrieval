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
// example enum type declaration
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
    INVALID = -1
};

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
                                        {SAY, "say"}})

class DialogueMessage
{
protected:
    CmdTypes m_type{INVALID};
    std::vector<std::string> m_params;
    std::string m_language;
    std::string m_query;
    std::string m_comment;

public:
    //NLOHMANN_DEFINE_TYPE_INTRUSIVE(DialogueMessage, m_type, m_params, m_language, m_query, m_comment)

    /**
     * Invalid constructor
     */
    DialogueMessage() = default;

    /**
     * Constructor
     * @param type
     * @param params
     * @param language
     * @param query
     * @param comment
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
     * Copy operator
     * @param anotherCommand the command to copy
     * @return the copied command
     */
    DialogueMessage& operator=(const DialogueMessage& anotherCommand);

    /**
     * @brief Get command type
     * @return the command type
     */
    [[nodiscard]] CmdTypes getType() const;

    /**
     * @brief Get command as a string
     * @return the command as a string
     */
    [[nodiscard]] std::string getTypeAsString() const;

    /**
     * @brief Get command parameters
     * @return the command parameters
     */
    [[nodiscard]] const std::vector<std::string>& getParams() const;

    /**
     * @brief Get command language
     * @return the command language
     */
    [[nodiscard]] const std::string& getLanguage() const;

    /**
     * @brief Get command query
     * @return the command query
     */
    [[nodiscard]] const std::string& getQuery() const;

    /**
     * @brief Get command comment
     * @return the command comment
     */
    [[nodiscard]] const std::string& getComment() const;

    /**
     * @brief Set command type
     * @param type the command type
     * @return true if the type is set, false otherwise - TODO: Decide if the return value is useful
     */
    bool setType(CmdTypes type);

    /**
     * @brief Set command parameters
     * @param params the command parameters
     * @return true if the parameters are set, false otherwise - TODO: Decide if the return value is useful
     */
    bool setParams(const std::vector<std::string>& params);

    /**
     * @brief Set command language
     * @param language the command language
     * @return true if the language is set, false otherwise - TODO: Decide if the return value is useful
     */
    bool setLanguage(const std::string& language);

    /**
     * @brief Set command query
     * @param query the command query
     * @return true if the query is set, false otherwise - TODO: Decide if the return value is useful
     */
    bool setQuery(const std::string& query);

    /**
     * @brief Set command comment
     * @param comment the command comment
     * @return true if the comment is set, false otherwise - TODO: Decide if the return value is useful
     */
    bool setComment(const std::string& comment);

};

inline void from_json(const json& j, DialogueMessage& msg) {
    printf("Ciaomeeeerdeeeeee");
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

