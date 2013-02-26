#ifndef medRegistrationWorkspace_H
#define medRegistrationWorkspace_H


#include <QtCore>

#include <medWorkspace.h>

class medTabbedViewContainers;
class medRegistrationWorkspacePrivate;

class medRegistrationWorkspace : public medWorkspace
{
    Q_OBJECT

public:
     medRegistrationWorkspace(QWidget *parent = 0);
    ~medRegistrationWorkspace(void);

    virtual QString identifier()  const;
    virtual QString description() const;
    static bool isUsable();
    void setupViewContainerStack ();

public slots:
    void patientChanged(int patientId);

private:
    medRegistrationWorkspacePrivate *d;
};

#endif // medRegistrationWorkspace_H