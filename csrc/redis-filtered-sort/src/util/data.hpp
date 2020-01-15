#ifndef __DATA_H
#define __DATA_H

#include <vector>
#include <map>
#include <string>
#include "./redis_commands.hpp"
#include "./util/filter_fn.hpp"
#include "./filters.hpp"

namespace ms {
    using namespace std;

    class Data {
        private:
            RedisCommander redis;
            vector<string> data;
            string dataKeyTemplate;
        public:
            string dataTemplate;

            Data(RedisCommander, string);
            ~Data();
            
            void loadList(string);
            void loadSet(string);
            void use(vector<string>);
            void save(string);
            void save(string, vector<string>);
            vector<pair<string, map<string,string>>> loadMetadata(vector<string> fields);
            size_t size();

            vector<string> sort(string);
            vector<string> sortMeta(string, string);
            vector<string> filter(string);
            vector<string> filterMeta(Filter);

    };
    
    Data::Data(RedisCommander redis, string metaTemplate) {
        this->redis = redis;
        this->dataKeyTemplate = metaTemplate;
    }
    
    Data::~Data() {
    }

}

#endif