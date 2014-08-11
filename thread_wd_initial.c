// Add thread_wd to Connection struct
    char thread_wd[1024];
    snprintf(thread_wd, 3, "%s", "./");
    printf("thread wd: %s\n", thread_wd);

// handling PWD: 
        if (strcmp(parsed[0], PWD) == 0) {
            printf("thread wd: %s\n", thread_wd);
            snprintf(response, MAX_MSG_LENGTH, "257 \"%s\" \n", (thread_wd + 1)); // +1 to ignore leading '.'
        }
// handling CWD:
        else if (strcmp(parsed[0], CWD) == 0) {

            struct stat s;
            char potential_path[1024];
            memset(potential_path, '\0', 1024);
            strncpy(potential_path, thread_wd, strlen(thread_wd));
            strncat(potential_path, parsed[1], 1024 - strlen(thread_wd));
            strncat(potential_path, "/", 2);
            printf("&&&&&&&&&&&&&&&&&&&&&& Potential path: %s\n", potential_path);
            
            // Thanks to http://stackoverflow.com/questions/9314586/c-faster-way-to-check-if-a-directory-exists
            int error_status = stat(potential_path, &s);
            printf("error status is %d\n:", error_status);
            if(error_status == -1) {
                printf("here -- error 1\n");
                snprintf(response, MAX_MSG_LENGTH, "%s %s: %s", "550", parsed[1], "No such file or directory\n");
            } 
            else {
                if(S_ISDIR(s.st_mode)) {
                    snprintf(thread_wd, 1024, "%s", potential_path);
                    snprintf(response, MAX_MSG_LENGTH, "%s", "250 CWD successful\n");
                } else {
                    printf("here -- error 2\n");
                    // can't just use perror
                    snprintf(response, MAX_MSG_LENGTH, "%s %s: %s", "550", parsed[1], "Not a directory\n");
                }
            }



        }
// and then when handling NLST, we only have to issue
    d = opendir(thread_wd);



