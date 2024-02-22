#pragma once

#include <QAtomicInt>
#include <QMutex>
#include <QThread>
#include <QThreadStorage>
#include <QWaitCondition>
#include <QtGlobal>

namespace CallOnce {
enum ECallOnce {
    CO_Request,
    CO_InProgress,
    CO_Finished
};

Q_GLOBAL_STATIC(QThreadStorage<QAtomicInt*>, once_flag)
} // namespace CallOnce

template <class Function>
inline static void qCallOnce(Function func, QBasicAtomicInt& flag)
{
    using namespace CallOnce;

    int protectFlag = flag.fetchAndStoreAcquire(flag.load());

    if (protectFlag == CO_Finished)
        return;
    if (protectFlag == CO_Request && flag.testAndSetRelaxed(protectFlag, CO_InProgress)) {
        func();
        flag.fetchAndStoreRelease(CO_Finished);
    } else {
        do {
            QThread::yieldCurrentThread();
        } while (!flag.testAndSetAcquire(CO_Finished, CO_Finished));
    }
}

template <class Function>
inline static void qCallOncePerThread(Function func)
{
    using namespace CallOnce;
    if (!once_flag()->hasLocalData()) {
        once_flag()->setLocalData(new QAtomicInt(CO_Request));
        qCallOnce(func, *once_flag()->localData());
    }
}
