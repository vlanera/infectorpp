/* Copyright (c) 2013 Dario Oliveri

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.*/

//----------------------------------------------------------------------------
/**
    Series of tests to make sure intended usage is possible and what should
    failing is actually failing. (well there are so many exceptions that
    writing full test suite will require a bit :/)
*/

#include <InfectorContainer.hpp>
#include <memory>
#include <iostream>
#include <cassert>

#ifndef INFECTOR_VS_DISABLE //TESTS 1,2,3-B,6 are disabled on VS

/// ////////////////////////////////////////////////
///                  TEST
///                    1
/// ////////////////////////////////////////////////
int CtorTotCounter = 0;
int CtorGoodCounter = 0;
int CtorBadCounter = 0;
struct BadClass{
    BadClass(){
        CtorTotCounter++;
        CtorBadCounter++;
        //std::cout<<"Bad Ctor"<<std::endl; //DISPLAY CONSTRUCTOR CALL
        throw std::exception();}
};
typedef std::unique_ptr<BadClass> bad_ptr;

struct GoodClass{
    static int counter;
    GoodClass(){
        CtorTotCounter++;
        CtorGoodCounter++;
        //std::cout<<"Good Ctor"<<std::endl; // DISPLAY CONSTRUCTOR CALL
        counter++;}
    ~GoodClass(){counter--;}
};
int GoodClass::counter = 0;

typedef std::unique_ptr<GoodClass> good_ptr;

struct GB{
    bad_ptr  bc;
    good_ptr gc;

    GB(good_ptr good, bad_ptr bad):
        bc(std::move(bad)),gc(std::move(good)){}
};


struct BG{
    bad_ptr  bc;
    good_ptr gc;

    BG(bad_ptr bad, good_ptr good):
        bc(std::move(bad)),gc(std::move(good)){ }
};

void LeakTest(){
    CtorTotCounter = 0;
    CtorGoodCounter = 0;
    CtorBadCounter = 0;
    GoodClass::counter = 0;
    {
        Infector::Container ioc;
        ioc.bindAsNothing<BadClass>();
        ioc.bindAsNothing<GoodClass>();
        ioc.bindAsNothing<GB>();
        ioc.bindAsNothing<BG>();
        ioc.wire<BadClass>();
        ioc.wire<GoodClass>();
        // C++ make no assumption about evaluation order of parameters
        // so I must test both orders.. and there's no guarantee that
        // evaluation order will be the same between 2 calls, so manual
        // attention must be paid here.
        ioc.wire<BG       ,BadClass,GoodClass>();
        ioc.wire<GB       ,GoodClass,BadClass>();
        //std::cout<<"\n3 CONSTRUCTORS CALLS MUST BE DISPLAYED:\n"<<std::endl;
        try{
        auto a = ioc.build<BG>();
        }catch(std::exception ex){

        }
        //std::cout<<"2nd call"<<std::endl;
        try{
        auto b = ioc.build<GB>();
        }catch(std::exception ex){

        }
    }
    assert("failed test 1" && GoodClass::counter == 0); // TEST MEMORY LEAK
    assert("failed test 1" && CtorTotCounter == 3);
    assert("failed test 1" && CtorBadCounter > 0 && CtorGoodCounter > 0 );
    GoodClass::counter = 0;
    CtorTotCounter = 0;
    CtorBadCounter = 0;
    CtorGoodCounter = 0;
    std::cout<<"\nend of test 1\n"<<std::endl;
}

/// ////////////////////////////////////////////////
///                  TEST
///                    2
/// ////////////////////////////////////////////////

int CCallBed = 0;
int CCallRoom = 0;

class IBed{
public:
    virtual void sleep() = 0;
    virtual ~IBed(){}
};

class IRoom{
public:
    virtual void interact() = 0;
    virtual ~IRoom(){}
};

class ComfortableBed: public virtual IBed{
public:
    ComfortableBed(){}
    virtual ~ComfortableBed(){}

    virtual void sleep(){
        CCallBed++;
        std::cout<<"yumm so comfortable!"<<std::endl;
    }
};

class BedRoom:public virtual IRoom{

    std::unique_ptr<IBed> myBed;

public:
    BedRoom(std::unique_ptr<IBed> b):myBed(std::move(b)){
    }
    virtual ~BedRoom(){}

    virtual void interact(){
        CCallRoom++;
        std::cout<<"interacting with room:"<<std::endl;
        myBed->sleep();
    }
};


void BedTest(){
    std::cout<<"ROOM AND COMFORTABLE BED\n"<<std::endl;
    Infector::Container ioc;

    ioc.bindAs<ComfortableBed,  IBed>();
    ioc.bindAs<BedRoom,         IRoom>();

    ioc.wire<ComfortableBed>();
    ioc.wire<BedRoom,           IBed>();

    auto room = ioc.build<IRoom>();
    room->interact();

    assert("test 2 failed" && CCallBed==1);
    assert("test 2 failed" && CCallRoom==1);

    std::cout<<"\nend of test 2\n"<<std::endl;
}

/// ////////////////////////////////////////////////
///                  TEST (DEPRECATED)
///                    3
/// ////////////////////////////////////////////////
/*struct Booom;
struct Kaaa{
public:
    Kaaa(std::unique_ptr<Booom> b){ (void) b;}
};


struct Booom{
public:
    Booom(std::unique_ptr<Kaaa> k){ (void) k;}
};

void CircularTest(){
    std::cout<<"CIRCULAR DEPENDENCY TEST\n"<<std::endl;
    Infector::Container ioc;
    ioc.bindAsNothing<Kaaa>();
    ioc.bindAsNothing<Booom>();
    ioc.wire<Kaaa,Booom>();
    ioc.wire<Booom,Kaaa>();

    try{
        auto kaboom = ioc.build<Kaaa>();
    }
    catch(std::exception &ex){
        std::cout<<ex.what()<<std::endl;
    }

    std::cout<<"\nend of test 3\n"<<std::endl;
}*/

/// ////////////////////////////////////////////////
///                  TEST
///                    3-B
/// ////////////////////////////////////////////////

struct Booom;
struct Kaaa{
public:
    Kaaa(std::unique_ptr<Booom> b){ (void) b;}
};


struct Booom{
public:
    Booom(std::unique_ptr<Kaaa> k){ (void) k;}
};

void CircularTest(){
    std::cout<<"CIRCULAR DEPENDENCY TEST\n"<<std::endl;
    Infector::Container ioc;
    ioc.bindAsNothing<Kaaa>();
    ioc.bindAsNothing<Booom>();

    bool ex1=false;
    bool ex2=false;

    try{
        ioc.wire<Kaaa,Booom>();
        ioc.wire<Booom,Kaaa>();
    }catch(std::exception &ex){
        ex1 = true;
        std::cout<<ex.what()<<std::endl;
    }

    try{
        auto kaboom = ioc.build<Kaaa>();
    }
    catch(std::exception &ex){
        ex2 = true;
        std::cout<<ex.what()<<std::endl;
    }

    assert("test 3b failed" && ex1);
    assert("test 3b failed" && ex2);

    std::cout<<"\nend of test 3\n"<<std::endl;
}
#endif
/// ////////////////////////////////////////////////
///                  TEST
///                    4
/// ////////////////////////////////////////////////

class IFoo {
public:
    virtual void letsFoo() = 0;
    virtual ~IFoo(){}
};

class IBar {
public:
    virtual void letsBar() = 0;
    virtual ~IBar(){}
};

class CFooBar: public virtual IFoo, public virtual IBar{
public:
    CFooBar(){std::cout<<"CFooBar ctor!"<<std::endl;}
    virtual ~CFooBar() {std::cout<<"FooBar  dctor"<<std::endl;}

    virtual void letsBar() override{
        std::cout<<"CFooBar : letsBar"<<std::endl;
    }
    virtual void letsFoo() override{
        std::cout<<"CFooBar : letsFoo"<<std::endl;
    }
};

class FooBarUser{
    std::shared_ptr<IFoo> myFoo;
    std::shared_ptr<IBar> myBar;
public:
    FooBarUser(){}
    FooBarUser(std::shared_ptr<IFoo> foo, std::shared_ptr<IBar> bar)
                : myFoo(foo)
                , myBar(bar)
    {  }
    virtual ~FooBarUser(){std::cout<<"FooBarUser dctor"<<std::endl;}

    void doSomething(){
        myFoo->letsFoo();
        myBar->letsBar();
    }
};

void SharedTest(){
    std::cout<<"SHARED OBJECTS TEST\n"<<std::endl;
    Infector::Container ioc;
    ioc.bindSingleAs<CFooBar, IFoo, IBar>();
    ioc.bindSingleAsNothing<FooBarUser>();

    ioc.wire<CFooBar>();
    ioc.wire<FooBarUser, IFoo, IBar>();

    try{
        auto user = ioc.buildSingle<FooBarUser>();
        user->doSomething();
    }
    catch(std::exception &ex){
        std::cout<<ex.what()<<std::endl;
    }

     std::cout<<"\nend of test 4\n"<<std::endl;
}

/// ////////////////////////////////////////////////
///                  TEST
///                    5
/// ////////////////////////////////////////////////

class IA{
public:
    virtual void doA()=0;
    virtual ~IA(){}
};

class IB{
public:
    virtual void doB()=0;
    virtual ~IB(){}
};

class IC{
public:
    virtual void doC()=0;
    virtual ~IC(){}
};

class concreteA: public virtual IA{
public:
    virtual void doA()override{ std::cout<<"A (A)"<<std::endl;}
    virtual ~concreteA(){}
};

class concreteABC:public virtual IA, public virtual IB, public virtual IC{
public:
    virtual ~concreteABC(){}
    virtual void doA()override{ std::cout<<"A (ABC)"<<std::endl;}
    virtual void doB()override{ std::cout<<"B (ABC)"<<std::endl;}
    virtual void doC()override{ std::cout<<"C (ABC)"<<std::endl;}
};

class concreteBC: public virtual IB, public virtual IC{
public:
    virtual ~concreteBC(){}
    virtual void doB()override{ std::cout<<"B (BC)"<<std::endl;}
    virtual void doC()override{ std::cout<<"C (BC)"<<std::endl;}
};

void MultiBaseFailure(){
    std::cout<<"MULTIPLE INHERITANCE FAILURE TEST\n"<<std::endl;
    Infector::Container ioc;
    ioc.bindSingleAs<concreteA, IA>();

    try{
        ioc.bindSingleAs<concreteABC, IA, IB, IC>();
    }catch(std::exception &ex){
        std::cout<<"ex 1"<<std::endl;
        std::cout<<ex.what()<<std::endl;
    }
    ioc.bindSingleAs<concreteBC,IB,IC>();

    ioc.wire<concreteA>();
    ioc.wire<concreteBC>();
    std::cout<<"step1"<<std::endl;
    try{
        ioc.wire<concreteABC>(); // that will never be used.. throw?

    }
    catch(std::exception &ex){
        std::cout<<"ex 2"<<std::endl;
        std::cout<<ex.what()<<std::endl;
    }
    std::cout<<"step2"<<std::endl;
    try{
        auto a=ioc.buildSingle<IA>();
        auto bcb=ioc.buildSingle<IB>();
        auto bcc=ioc.buildSingle<IC>();
        a->doA();
        bcb->doB();
        bcc->doC();
    }
    catch(std::exception &ex){
        std::cout<<"ex 3"<<std::endl;
        std::cout<<ex.what()<<std::endl;
    }

     std::cout<<"\nend of test 5\n"<<std::endl;
}

#ifndef INFECTOR_VS_DISABLE
/// ////////////////////////////////////////////////
///                  TEST
///                    6
/// ////////////////////////////////////////////////


void InstantiateConcreteMustFail(){
    std::cout<<"CONCRETE INSTANTIATE\n"<<std::endl;
    Infector::Container ioc;

    ioc.bindAs<ComfortableBed,  IBed>();

    ioc.wire<ComfortableBed>();

    auto bed = ioc.build<IBed>();
    bool testPass = false;
    try{
        ioc.build<ComfortableBed>();
    }
    catch(std::exception &ex){
        std::cout<<"ex 1"<<std::endl;
        std::cout<<ex.what()<<std::endl;
        testPass=true;
    }

    assert("test 6 failed" && testPass == true);

    std::cout<<"\nend of test 6\n"<<std::endl;
}
#endif


/// ////////////////////////////////////////////////
///                  TEST
///                    7
/// ////////////////////////////////////////////////
int CtorTotCounterVS = 0;
int CtorGoodCounterVS = 0;
int CtorBadCounterVS = 0;
struct BadClassVS{
    BadClassVS(){
        CtorTotCounterVS++;
        CtorBadCounterVS++;
        //std::cout<<"Bad Ctor"<<std::endl; //DISPLAY CONSTRUCTOR CALL
        throw std::exception();}
};
typedef std::shared_ptr<BadClassVS> badVS_ptr;

struct GoodClassVS{
    static int counter;
    GoodClassVS(){
        CtorTotCounterVS++;
        CtorGoodCounterVS++;
        //std::cout<<"Good Ctor"<<std::endl; // DISPLAY CONSTRUCTOR CALL
        counter++;}
    ~GoodClassVS(){counter--;}
};
int GoodClassVS::counter = 0;

typedef std::shared_ptr<GoodClassVS> goodVS_ptr;

struct GBVS{
    badVS_ptr  bc;
    goodVS_ptr gc;

    GBVS(goodVS_ptr good, badVS_ptr bad):
        bc((bad)),gc((good)){}
};


struct BGVS{
    badVS_ptr  bc;
    goodVS_ptr gc;

    BGVS(badVS_ptr bad, goodVS_ptr good):
        bc((bad)),gc((good)){ }
};

void LeakTest2(){
    CtorTotCounterVS = 0;
    CtorGoodCounterVS = 0;
    CtorBadCounterVS = 0;
    GoodClassVS::counter = 0;
    {
        Infector::Container ioc;
        ioc.bindSingleAsNothing<BadClassVS>();
        ioc.bindSingleAsNothing<GoodClassVS>();
        ioc.bindSingleAsNothing<GBVS>();
        ioc.bindSingleAsNothing<BGVS>();
        ioc.wire<BadClassVS>();
        ioc.wire<GoodClassVS>();
        // C++ make no assumption about evaluation order of parameters
        // so I must test both orders.. and there's no guarantee that
        // evaluation order will be the same between 2 calls, so manual
        // attention must be paid here.
        ioc.wire<BGVS       ,BadClassVS,GoodClassVS>();
        ioc.wire<GBVS       ,GoodClassVS,BadClassVS>();
        //std::cout<<"\n3 CONSTRUCTORS CALLS MUST BE DISPLAYED:\n"<<std::endl;
        try{
        auto a = ioc.buildSingle<BGVS>();
        }catch(std::exception &ex){
            std::cout<<"ex 1"<<std::endl;
            std::cout<<ex.what()<<std::endl;
        }
        //std::cout<<"2nd call"<<std::endl;
        try{
        auto b = ioc.buildSingle<GBVS>();
        }catch(std::exception &ex){
            std::cout<<"ex 2"<<std::endl;
            std::cout<<ex.what()<<std::endl;
        }
    }
    assert("failed test 7" && GoodClassVS::counter == 0); // TEST MEMORY LEAK
    assert("failed test 7" && CtorTotCounterVS == 3);
    assert("failed test 7" && CtorBadCounterVS > 0 && CtorGoodCounterVS > 0 );
    GoodClassVS::counter = 0;
    CtorTotCounterVS = 0;
    CtorBadCounterVS = 0;
    CtorGoodCounterVS = 0;
    std::cout<<"\nend of test 7\n"<<std::endl;
}


/// ////////////////////////////////////////////////
///                  TEST
///                    8
/// ////////////////////////////////////////////////

int CCallBedVS = 0;
int CCallRoomVS = 0;

class IBedVS{
public:
    virtual void sleep() = 0;
    virtual ~IBedVS(){}
};

class IRoomVS{
public:
    virtual void interact() = 0;
    virtual ~IRoomVS(){}
};

class ComfortableBedVS: public virtual IBedVS{
public:
    ComfortableBedVS(){}
    virtual ~ComfortableBedVS(){}

    virtual void sleep(){
        CCallBedVS++;
        std::cout<<"yumm so comfortable!"<<std::endl;
    }
};

class BedRoomVS:public virtual IRoomVS{

    std::shared_ptr<IBedVS> myBed;

public:
    BedRoomVS(std::shared_ptr<IBedVS> b):myBed(std::move(b)){
    }
    virtual ~BedRoomVS(){}

    virtual void interact(){
        CCallRoomVS++;
        std::cout<<"interacting with room:"<<std::endl;
        myBed->sleep();
    }
};

void BedTest2(){
    std::cout<<"ROOM AND COMFORTABLE BED 2\n"<<std::endl;
    Infector::Container ioc;

    ioc.bindSingleAs<ComfortableBedVS,  IBedVS>();
    ioc.bindSingleAs<BedRoomVS,         IRoomVS>();

    ioc.wire<ComfortableBedVS>();
    ioc.wire<BedRoomVS,           IBedVS>();

    auto room = ioc.buildSingle<IRoomVS>();
    room->interact();

    assert("test 8 failed" && CCallBedVS==1);
    assert("test 8 failed" && CCallRoomVS==1);

    std::cout<<"\nend of test 8\n"<<std::endl;
}


/// ////////////////////////////////////////////////
///                  TEST
///                   9
/// ////////////////////////////////////////////////

struct BooomVS;
struct KaaaVS{
public:
    KaaaVS(std::shared_ptr<BooomVS> b){ (void) b;}
};


struct BooomVS{
public:
    BooomVS(std::shared_ptr<KaaaVS> k){ (void) k;}
};

void CircularTest2(){
    std::cout<<"CIRCULAR DEPENDENCY TEST\n"<<std::endl;
    Infector::Container ioc;
    ioc.bindSingleAsNothing<KaaaVS>();
    ioc.bindSingleAsNothing<BooomVS>();

    bool ex1=false;
    bool ex2=false;

    try{
        ioc.wire<KaaaVS,BooomVS>();
        ioc.wire<BooomVS,KaaaVS>();
    }catch(std::exception &ex){
        ex1 = true;
        std::cout<<"ex1"<<std::endl;
        std::cout<<ex.what()<<std::endl;
    }

    try{
        auto kaboom = ioc.buildSingle<KaaaVS>();
    }
    catch(std::exception &ex){
        ex2 = true;
        std::cout<<"ex2"<<std::endl;
        std::cout<<ex.what()<<std::endl;
    }

    assert("test 9 failed" && ex1);
    assert("test 9 failed" && ex2);

    std::cout<<"\nend of test 9\n"<<std::endl;
}



/// ////////////////////////////////////////////////
///                   MAIN
///
/// ////////////////////////////////////////////////

int main(){

    /** Unique pointers does not take additional memory. */
    std::cout<<"sizeof(unique_ptr<int>):"<<(int)sizeof(std::unique_ptr<int>)<<std::endl;
    std::cout<<"sizeof(int*):"<<(int)sizeof(int*)<<std::endl;

    /** Test to prove that Infector does not cause memory leaks due to
    *   unkown evaluation order of constructors' parameters when construcors
    *   throws exceptions. This is a big advantage.*/
#ifndef INFECTOR_VS_DISABLE  //disabled on visual studio due to a compiler bug
    LeakTest();
    /** Basic usage test, no shared objects. Creates a small object graph
    *   with unique ownership semantics.*/
    BedTest();
    /** This test assure that exception is thrown in case there's a circular
    *   dependency. */
    CircularTest();
#endif
    /** Basic usage test this time shared objects are used and multiple
    *   inheritance is tested.*/
    SharedTest();
    /** Multiple inheritance test failure.*/
    MultiBaseFailure();
#ifndef INFECTOR_VS_DISABLE  //disabled on visual studio due to a compiler bug
    /** Can instantiate only bound types. Verify that.*/
    InstantiateConcreteMustFail();
#endif

    /**Same as Leak test1, but use shared_ptr instead of unique_ptr*/
    LeakTest2();

    /**Same as BedTest, but use shared_ptr instead of unique_ptr*/
    BedTest2();

    /**Same as Circular test, but use shared_ptr instead of unique_ptr*/
    CircularTest2();

    return 0;
}
