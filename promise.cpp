#include "promise.h"

#include <QDebug>

Promise::Promise(QObject *parent) :
    QObject(parent)
{
    s = pending;
}

Promise::Promise(QList<Promise *> promises, QObject *parent) :
    Promise(parent)
{
    tasksCount = promises.size();
    datas.reserve(tasksCount);
    int i = 0;
    if(tasksCount == 0){
        this->resolve();
    }
    for(auto p : promises){
        p->setParent(this);
        datas.push_back(QVariant());
        p->then([=](const QVariant& data){
            if(s == pending){
                tasksCount--;
                datas.replace(i, data);
                if(tasksCount == 0){
                    resolve(QVariant(datas));
                }
            }
        }, [=](const QVariant& data){
            if(s == pending){
                reject(data);
            }
        });
        i++;
    }
}

Promise* Promise::pipe(const Monad& suc,
                        const Monad& f)
{
    Promise* p = new Promise(this->parent());
    this->then([=](QVariant data){
        suc(data)->then([=](const QVariant& data){
            p->resolve(data);
        }, [=](const QVariant& error){
            p->reject(error);
        }, [=](const QVariant& progress){
            p->advance(progress);
        });
    }, [=](QVariant error){
        f(error)->then([=](const QVariant& data){
            p->resolve(data);
        }, [=](const QVariant& error){
            p->reject(error);
        }, [=](const QVariant& progress){
            p->advance(progress);
        });
    });
    return p;
}

Promise* Promise::pipe(const Monad& suc)
{
    Promise* p = new Promise(this->parent());
    this->then([=](QVariant data){
        suc(data)->then([=](const QVariant& data){
            p->resolve(data);
        }, [=](const QVariant& error){
            p->reject(error);
        }, [=](const QVariant& progress){
            p->advance(progress);
        });
    }, [=](QVariant error){
        p->reject(error);
    });
    return p;
}

Promise* Promise::then(const std::function<void(const QVariant&)> &suc)
{
    if(s == pending) connect(this, &Promise::done, suc);
    else if(s == success) suc(this->data);
    return this;
}

Promise* Promise::then(const std::function<void(const QVariant&)>& suc, const std::function<void(const QVariant&)>& f)
{
    if(s == pending){
        connect(this, &Promise::done, suc);
        connect(this, &Promise::fail, f);
    } else if(s == success) suc(this->data);
    else f(this->data);
    return this;
}

Promise* Promise::then(const std::function<void(const QVariant&)>& suc, const std::function<void(const QVariant&)>& f,
                        const std::function<void(const QVariant&)>& p)
{
    if(s == pending){
        connect(this, &Promise::done, suc);
        connect(this, &Promise::fail, f);
        connect(this, &Promise::progress, p);
    }else if(s == success) suc(this->data);
    else f(this->data);
    return this;
}

void Promise::resolve(const QVariant& data)
{
    if(s != pending){
        return;
    }else{
        s = success;
        emit done(data);
        this->data = data;
        deleteLater();
    }
}

void Promise::reject(const QVariant& data)
{
    if(s != pending){
        return;
    }else{
        s = failure;
        emit fail(data);
        this->data = data;
        deleteLater();
    }
}

void Promise::advance(const QVariant& data)
{
    emit progress(data);
}

Promise::State Promise::state(){
    return s;
}
