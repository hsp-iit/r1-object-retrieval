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

DialogueMessage::DialogueMessage(CmdTypes type, const std::vector<std::string>& params, const std::string& language, const std::string& query):
        m_type(type),
        m_params(params),
        m_language(language)
{
}

DialogueMessage& DialogueMessage::operator=(const DialogueMessage& anotherCommand)
{
    m_type = anotherCommand.m_type;
    m_params = anotherCommand.m_params;
    m_language = anotherCommand.m_language;

    return *this;
}

const std::vector<std::string>& DialogueMessage::getParams() const
{
    return m_params;
}

CmdTypes DialogueMessage::getType() const
{
    return m_type;
}

const std::string& DialogueMessage::getLanguage() const
{
    return m_language;
}

const std::string& DialogueMessage::getQuery() const
{
    return m_query;
}

bool DialogueMessage::setType(CmdTypes type)
{
    m_type = type;
    return true;
}

bool DialogueMessage::setParams(const std::vector<std::string>& params)
{
    m_params = params;
    return true;
}

bool DialogueMessage::setLanguage(const std::string& language)
{
    m_language = language;
    return true;
}

bool DialogueMessage::setQuery(const std::string& query)
{
    m_query = query;
    return true;
}
