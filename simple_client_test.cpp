/*
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif*/
//g++ simple_client_test.cpp -pthread -lmongoclient -lboost_thread -lboost_system -lboost_regex -o simple_client_test
#include "mongo/client/dbclient.h"  // the mongo c++ driver

#include <iostream>

#ifndef verify
#define verify(x) MONGO_verify(x)
#endif

using namespace std;
using namespace mongo;
//using mongo::BSONObj;
//using mongo::BSONObjBuilder;

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cout << "usage: " << argv[0] << " [MONGODB_URI]" << std::endl;
        return EXIT_FAILURE;
    }
    
    mongo::client::GlobalInstance instance;
    if (!instance.initialized()) {
        std::cout << "failed to initialize the client driver: " << instance.status() << std::endl;
        return EXIT_FAILURE;
    }

    std::string uri = argc == 2 ? argv[1] : "mongodb://127.0.0.1:27017";
    std::string errmsg;

    ConnectionString cs = ConnectionString::parse(uri, errmsg);

    if (!cs.isValid()) {
        std::cout << "Error parsing connection string " << uri << ": " << errmsg << std::endl;
        return EXIT_FAILURE;
    }

    boost::scoped_ptr<DBClientBase> conn(cs.connect(errmsg));
    if (!conn) {
        cout << "couldn't connect : " << errmsg << endl;
        return EXIT_FAILURE;
    }

    const char* ns = "test.first";

    conn->dropCollection(ns);
    
    //clean up old data from any previous tesets
    conn->remove(ns, BSONObj());
    verify(conn->findOne(ns, BSONObj()).isEmpty()); //verify逻辑表达式，判断是否还存在test.first，是否clenn up成功
    
    //test insert
    cout << "(1) test insert----" << endl;
    conn->insert(ns, 
                 BSON("name" << "joe" 
                      << "pwd" << "123456" 
                      << "age" << 20));
    verify(!conn->findOne(ns, BSONObj()).isEmpty());     //判断是否添加成功
    cout << "insert data : " << conn->findOne(ns, BSONObj()) << endl;
    cout << "insert success!" << endl;

    // test remove
    cout << "(2) test remove----" << endl;
    conn->remove(ns, BSONObj());
    verify(conn->findOne(ns, BSONObj()).isEmpty());
    cout << "remove success!" << endl;
    
    // insert, findOne testing
    conn->insert(ns, 
                 BSON("name" << "joe" 
                      << "pwd" << "234567" 
                      << "age" << 21));
    {
        BSONObj res = conn->findOne(ns, BSONObj());
        verify(strstr(res.getStringField("name"), "joe"));
        verify(!strstr(res.getStringField("name2"), "joe"));
        verify(21 == res.getIntField("age"));
    }
    cout << "insert data : " << conn->findOne(ns, BSONObj()) <<endl;

    // test update
    {
        BSONObj res = conn->findOne(ns, BSONObjBuilder().append("name", "joe").obj());
        verify(!strstr(res.getStringField("name2"), "jeo"));
        
        BSONObj after = BSONObjBuilder().appendElements(res).append("name2", "w").obj();
        cout << "(3) test update name joe add name2 name3----" << endl;
        //upsert type1 update method
        conn->update(ns, BSONObjBuilder().append("name", "joe").obj(), after);
        //res = conn->findOne(ns, BSONObjBuilder().append("name2", "w").obj());
        verify(!strstr(res.getStringField("name2"), "joe"));
        verify(conn->findOne(ns, BSONObjBuilder().append("name", "joe2").obj()).isEmpty());
        //cout << " update1 data: " << conn->findOne(ns, BSONObj()) << endl;
        cout << " update1 data : " << conn->findOne(ns, Query("{name2:'w'}")) << endl;
        
        //upsert type2 update method 更改指定的某个属性值 
        const string TEST_NS = "test.first";
        conn->update("test.first", res, BSON("$set" << BSON("age" << 11)));
        cout << " update2 data : " << conn->findOne(ns, BSONObj()) << endl;
        res = conn->findOne(ns, BSONObjBuilder().append("name", "joe").obj());

        //upsert type3 update method
        try
        {
            after = BSONObjBuilder().appendElements(res).append("name3", "h").obj();
            conn->update(ns, BSONObjBuilder().append("name", "joe").obj(), after);
        }
        catch (OperationException&)
        {
            cout << " update error: " << conn->getLastErrorDetailed().toString() << endl;
        }
        verify(!conn->findOne(ns, BSONObjBuilder().append("name", "joe").obj()).isEmpty());
        //cout << " update3 data: " << conn->findOne(ns, BSONObj()) << endl;
        cout << " update3 data : " << conn->findOne(ns, Query("{name3:'h'}")) << endl;

        cout << "(4) test query-----" << "\n query data : " 
            << conn->findOne(ns, BSONObjBuilder().append("name", "joe").obj()) << endl;
        cout << " Query data : " << conn->findOne(ns, Query("{name:'joe'}")) << endl;
    }

    return 0;
}
