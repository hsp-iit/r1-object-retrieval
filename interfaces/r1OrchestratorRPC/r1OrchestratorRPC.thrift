service r1OrchestratorRPC {
    
    /**
     * the robot starts to search for 'what'
     * @param string containing the object to search
     * @return true/false on success/failure
     */
    bool searchObject(1: string what);

    /**
     * the robot starts to search for 'what' in location 'where'
     * @param string containing the object to search
     * @param string containing the location where to search
     * @return true/false on success/failure
     */
    bool searchObjectLocation(1: string what, 2: string where);

    /**
     * stops the search
     * @return true/false on success/failure
     */
    bool stopSearch();

    /**
     * resets the search
     * @return true/false on success/failure
     */
    bool resetSearch();

    /**
     * resets the search and navigates the robot home
     * @return true/false on success/failure
     */
    bool resetHome();

    /**
     * resumes stopped search
     * @return true/false on success/failure
     */
    bool resume();

    /**
     * @return string containing the current status of the search
     */
    string status();

    /**
     * @return string containing the object of the current search
     */
    string what();

    /**
     * @return string containing the location of the current search
     */
    string where();

    /**
     * sets the robot in navigation position
     * @return true/false on success/failure
     */
    bool navpos();
    
    /**
     * navigates the robot to 'location' if it is valid
     * @param string containing the location to reach
     * @return true/false on success/failure
     */
    bool go(1: string location);
    
    /**
     * the sentence is synthesized and played by the audio player
     * @param string containing the sentence to be said
     * @return true/false on success/failure
     */
    bool say(1: string sentence);
    
    /**
     * a previously stored sentence is accessed through the corresponfing key and it is played by the audio player
     * example: tell barzelletta
     * @param string containing the key corresponding to the sentence to be said
     * @return true/false on success/failure
     */
    bool tell(1: string key);
    
    /**
     * make the robot move as defined in the corresponding ini file containing the choreography
     * example: dance hi
     * @param string containing the name of the ini file containing the choreography
     * @return true/false on success/failure
     */
    bool dance(1: string motion);

}