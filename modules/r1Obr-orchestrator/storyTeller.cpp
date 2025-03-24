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

#include "storyTeller.h"

YARP_LOG_COMPONENT(STORY_TELLER, "r1_obr.orchestrator.storyTeller")

bool StoryTeller::configure(ResourceFinder& rf)
{
   m_stories_file = "stories_to_read.ini";
   m_active = true;

   if(!rf.check("STORY_TELLER"))
    {
        yCWarning(STORY_TELLER,"STORY_TELLER section missing in ini file. Story Teller will not be active");
        m_active = false;
        return true;
    }
    else
    {
        Searchable& config = rf.findGroup("STORY_TELLER");
        if(config.check("stories_file")) {m_stories_file = config.find("stories_file").asString();}

        string path_to_file = rf.findFileByName(m_stories_file);
        ifstream file;
        file.open(path_to_file);

        if (!file)
        {
            yCError(STORY_TELLER) << m_stories_file.c_str() << "could not be opened";
            return false;
        }
            
        for (string line; getline(file, line);)
        {
            
            if (!line.empty() && (line[0] == ';' || line[0] == '#')) // if a comment
            {
                // allow both ; and # comments at the start of a line
            }
            else if (!line.empty()) 
            {
                /* Not a comment, must be a key[=: ]value pair */
                size_t endKey = line.find_first_of("=: ");
                if (endKey != string::npos) 
                {
                    string key = line.substr(0, endKey);
                    string value = line.substr(endKey + 1);

                    //ltrim key
                    key.erase(key.begin(), find_if(key.begin(), key.end(), [](unsigned char ch) { return !isspace(ch); }));
                    //rtrim key
                    key.erase(find_if(key.rbegin(), key.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), key.end());

                    //ltrim key
                    value.erase(value.begin(), find_if(value.begin(), value.end(), [](unsigned char ch) { return !isspace(ch); }));
                    //rtrim key
                    value.erase(find_if(value.rbegin(), value.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), value.end());

                    m_stories.insert({key, value});
                }
            }
        }  
    }

    return true;
}



/****************************************************************/
bool StoryTeller::getStory(const string& key, string& story)
{  
    if (m_stories.find(key) != m_stories.end())
    {
        story = m_stories.at(key);
        return true;
    }

    return false;
}
