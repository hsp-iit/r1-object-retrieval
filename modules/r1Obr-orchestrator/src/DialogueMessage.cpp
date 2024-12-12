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

#include "DialogueMessage.h"
#include <utility>


dlgmsg::DialogueMessage::DialogueMessage(CmdTypes type, const std::vector<std::string>& params, const std::string& language, const std::string& query, const std::string& comment) :
        m_type(type),
        m_params(params),
        m_language(language),
        m_query(query),
        m_comment(comment)
{
}

dlgmsg::DialogueMessage& dlgmsg::DialogueMessage::operator=(const dlgmsg::DialogueMessage& anotherCommand)
{
    m_type = anotherCommand.m_type;
    m_params = anotherCommand.m_params;
    m_language = anotherCommand.m_language;
    m_query = anotherCommand.m_query;
    m_comment = anotherCommand.m_comment;

    return *this;
}

const std::vector<std::string>& dlgmsg::DialogueMessage::getParams() const
{
    return m_params;
}

dlgmsg::CmdTypes dlgmsg::DialogueMessage::getType() const
{
    return m_type;
}

std::string dlgmsg::DialogueMessage::getTypeAsString() const
{
    static const std::map<CmdTypes, std::string> cmdTypeToString = {
            {CmdTypes::GO, "go"},
            {CmdTypes::SEARCH, "search"},
            {CmdTypes::STOP, "stop"},
            {CmdTypes::RESUME, "resume"},
            {CmdTypes::RESET, "reset"},
            {CmdTypes::WHERE, "where"},
            {CmdTypes::WHAT, "what"},
            {CmdTypes::STATUS, "status"},
            {CmdTypes::NAVPOS, "navpos"},
            {CmdTypes::SAY, "say"},
            {CmdTypes::INVALID, "invalid"}
    };

    return cmdTypeToString.at(m_type);
}

const std::string& dlgmsg::DialogueMessage::getLanguage() const
{
    return m_language;
}

const std::string& dlgmsg::DialogueMessage::getQuery() const
{
    return m_query;
}

const std::string& dlgmsg::DialogueMessage::getComment() const
{
    return m_comment;
}

bool dlgmsg::DialogueMessage::setType(dlgmsg::CmdTypes type)
{
    m_type = type;
    return true;
}

bool dlgmsg::DialogueMessage::setParams(const std::vector<std::string>& params)
{
    m_params = params;
    return true;
}

bool dlgmsg::DialogueMessage::setLanguage(const std::string& language)
{
    m_language = language;
    return true;
}

bool dlgmsg::DialogueMessage::setQuery(const std::string& query)
{
    m_query = query;
    return true;
}

bool dlgmsg::DialogueMessage::setComment(const std::string& comment)
{
    m_comment = comment;
    return true;
}
