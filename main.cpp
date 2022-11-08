#include <curl/curl.h>
#include <iostream>
#include <cstring>
#include <string>
#include <queue>
#include <stack>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>

#define HREF_BYPASS 9
#define BASE_WIKI_LINK "https://en.wikipedia.org"

size_t parse(char*, size_t, size_t, void*);

std::queue<std::string> parsed;
std::queue<std::string> queue;

//key is a wiki page link, value is how we got there
std::unordered_map<std::string, std::string> discovered; 

void explore_outlink(CURL* curl, std::string outlink, std::string target)
{
    std::string barelink = outlink;
    outlink = BASE_WIKI_LINK + outlink;

    //set options
    curl_easy_setopt(curl, CURLOPT_URL, outlink.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, parse);

    //perform out option
    CURLcode result = curl_easy_perform(curl);
    if(result != CURLE_OK)
    {
        printf("curl download failed\n");
    }

    //look at all links that were parsed
    while(!parsed.empty())
    {
        std::string curr = parsed.front();
        parsed.pop();

        if(curr == target)
        {
            //trace back the path and print
            //I would normally make a function for this but im too lazy
            std::stack<std::string> path;
            std::string trace = barelink;
            path.push(curr);
            path.push(barelink);
            while(1)
            {
                auto next = discovered.find(trace);
                if(next == discovered.end())
                {
                    std::cout << "break in the traceback chain. exiting.\n";
                    exit(-1);
                }
                else
                {
                    if(next->second == "x") break;
                    path.push(next->second);
                    trace = next->second;
                }
            }

            std::cout << "path found :\n";

            while(!path.empty())
            {
                std::cout << path.top().substr(6) << "\n";
                path.pop();
            }

            exit(0);
        }

        auto found = discovered.find(curr);

        if(found == discovered.end())
        {
            queue.push(curr);
            discovered.insert({curr, barelink});
        }
        else continue;
    }
}

size_t parse(char* buffer, size_t itemsize, size_t nitems, void* ignore)
{
    size_t bytes = itemsize * nitems;
    std::string keystr = "<a href=\"/wiki/";

    for(int i = 0; i < bytes-keystr.length(); i++)
    {
        //see if this char is the start of keystr
        bool match = true;
        for(int j = 0; j < keystr.length(); j++)
        {
            if(buffer[i+j] != keystr[j])
            {
                match = false;
                break;
            }
        }

        //past this line only matches should go through
        if(!match) continue;

        //search for the end of the href
        int end = 0;
        for(int j = i+keystr.length(); j < bytes; j++)
        {
            if(buffer[j] == '\"')
            {
                end = j;
                break;
            }
        }
        
        //ensure that we arent overshooting the end of buffer
        if(!end) continue;

        //get a copy of the link as a C string
        int linklen = end-i-HREF_BYPASS;
        char outlink_cstr[linklen+1];
        std::memcpy(outlink_cstr, &buffer[i+HREF_BYPASS], linklen);
        outlink_cstr[linklen] = '\0';

        //convert the C string to a cpp string
        std::string outlink(outlink_cstr);

        parsed.push(outlink);
    }

    return bytes;
}

int main(int argc, char* argv[])
{
    if(argc-1 != 2)
    {
        std::cout << "improper calling form.\n";
        exit(-1);
    }

    std::string base(argv[1]);
    std::string target(argv[2]);

    //config
    CURL* curl = curl_easy_init();
    if(!curl)
    {
        printf("curl init failed\n");
        return -1;
    }

    queue.push(base);
    discovered.insert({base, "x"});

    while(!queue.empty())
    {
        std::string outlink = queue.front();
        queue.pop();

        explore_outlink(curl, outlink, target);
    }

    curl_easy_cleanup(curl);
    return 0;
}