// Hmmmmm
#include "btree_ext.hpp"

TestHarness *harness = 0;

BTreeNode* test_get_node(u_int64_t page_ptr){
    #ifdef INFO
    printf("Get Node: %lu\n", page_ptr);
    #endif
    // return harness->pages[page_ptr];
    return (BTreeNode*)page_ptr;
}

u_int64_t test_new_page(BTreeNode* page_ptr){
    #ifdef INFO
    printf("New Page: %lu\n", page_ptr);
    #endif
    // harness->pages[(u_int64_t) page_ptr] = page_ptr;
    return (u_int64_t) page_ptr;
}

void test_deallocate_page(u_int64_t page_ptr){
    #ifdef INFO
    printf("Deallocate Page: %lu\n", page_ptr);
    #endif
    // free((void*)page_ptr);
}

void initTestHarness(){
    harness = new TestHarness();
    harness->tree = (BTree*)malloc(sizeof(BTree));
    harness->tree->root = 0;
    harness->tree->get_node = test_get_node;
    harness->tree->new_page = test_new_page;
    harness->tree->dealloc_page = test_deallocate_page;
}

void add(std::string key, std::string val){
    Bytes key_bytes = Bytes{(u_int16_t)(((u_int8_t) key.size() + 1)), (u_int8_t*) key.c_str()};
    Bytes val_bytes = Bytes{(u_int16_t)(((u_int8_t) val.size() + 1)), (u_int8_t*) val.c_str()};
    // printf("INSERTING: size: %d, string: %s\n", key_bytes.len, key_bytes.data);
    harness->reference_map[key] =  val;
    BTInsert(harness->tree, key_bytes, val_bytes);
}

char* find(std::string key){
    Bytes key_bytes = Bytes{(u_int16_t) key.size(), (u_int8_t*) key.c_str()};
    return (char*)BTFind(harness->tree, key_bytes).data;
}

bool delete_(std::string key){
    Bytes key_bytes = Bytes{(u_int16_t) key.size(), (u_int8_t*) key.c_str()};
    harness->reference_map.erase(key);
    return BTDelete(harness->tree, key_bytes);
}

void testBTree(){
    initTestHarness();
    printf("- Testing BTree:\n");
    add(std::string("hello"), std::string("world!"));

    printf("hmmm\n");

    printf("find: %s\n", find(std::string("hello")));
    delete_(std::string("hello"));
    
    add(std::string("Zhello1"), std::string("new world!!!!"));
    add(std::string("1"), std::string("no!"));
    add(std::string("hello"), std::string("world!"));

    printf("find: %s\n", find(std::string("hello")));
    printf("find: %s\n", find(std::string("Zhello1")));
    printf("find: %s\n", find(std::string("1")));

    delete_(std::string("hello"));
    printf("find: %s\n", find(std::string("hello")));
    add(std::string("hello"), std::string("world2!"));
    printf("find: %s\n", find(std::string("hello")));
    printf("find: %s\n", find(std::string("Zhello1")));

    for (int i = 0; i < 10000; ++i){
        // printf("root node size: %hu\n", BTNodeSize(harness->tree->get_node(harness->tree->root)));
        char string[] = "000000";
        string[5] = (char)(i % 10 + 48); 
        string[4] = (char)((i  % 100) / 10  + 48); 
        string[3] = (char)((i  % 1000) / 100  + 48); 
        string[2] = (char)((i  % 10000) / 1000  + 48); 
        string[1] = (char)((i  % 100000) / 10000  + 48); 
        string[1] = (char)((i  % 1000000) / 100000  + 48); 

        
        add(std::string(string), std::string("upupupdate!"));
        assert(memcmp(find(std::string(string)), "upupupdate!", 12) == 0);
        add(std::string(string), std::string("0a1b2c3d4!"));
        assert(memcmp(find(std::string(string)), "0a1b2c3d4!", 11) == 0);
        delete_(std::string(string));
        assert(find(string) == 0);
        add(std::string(string), std::string("upupupdate!"));
    }
    printf("Test succesfully mastered!");
}