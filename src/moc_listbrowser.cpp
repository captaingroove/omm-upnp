/****************************************************************************
** ListBrowser meta object code from reading C++ file 'listbrowser.h'
**
** Created: Tue Dec 5 17:33:45 2006
**      by: The Qt MOC ($Id: qt/moc_yacc.cpp   3.3.6   edited Mar 8 17:43 $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "listbrowser.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.3.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *ListBrowser::className() const
{
    return "ListBrowser";
}

QMetaObject *ListBrowser::metaObj = 0;
static QMetaObjectCleanUp cleanUp_ListBrowser( "ListBrowser", &ListBrowser::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString ListBrowser::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ListBrowser", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString ListBrowser::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ListBrowser", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* ListBrowser::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = Page::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "title", &static_QUType_ptr, "Title", QUParameter::In }
    };
    static const QUMethod slot_0 = {"addEntry", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "title", &static_QUType_ptr, "Title", QUParameter::In }
    };
    static const QUMethod slot_1 = {"delEntry", 1, param_slot_1 };
    static const QUParameter param_slot_2[] = {
	{ 0, &static_QUType_ptr, "Title", QUParameter::Out }
    };
    static const QUMethod slot_2 = {"getCurrent", 1, param_slot_2 };
    static const QUParameter param_slot_3[] = {
	{ "entry", &static_QUType_ptr, "QListViewItem", QUParameter::In }
    };
    static const QUMethod slot_3 = {"showPopupMenu", 1, param_slot_3 };
    static const QUMethod slot_4 = {"clear", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "addEntry(Title*)", &slot_0, QMetaData::Public },
	{ "delEntry(Title*)", &slot_1, QMetaData::Public },
	{ "getCurrent()", &slot_2, QMetaData::Public },
	{ "showPopupMenu(QListViewItem*)", &slot_3, QMetaData::Protected },
	{ "clear()", &slot_4, QMetaData::Protected }
    };
    metaObj = QMetaObject::new_metaobject(
	"ListBrowser", parentObject,
	slot_tbl, 5,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_ListBrowser.setMetaObject( metaObj );
    return metaObj;
}

void* ListBrowser::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "ListBrowser" ) )
	return this;
    return Page::qt_cast( clname );
}

bool ListBrowser::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: addEntry((Title*)static_QUType_ptr.get(_o+1)); break;
    case 1: delEntry((Title*)static_QUType_ptr.get(_o+1)); break;
    case 2: static_QUType_ptr.set(_o,getCurrent()); break;
    case 3: showPopupMenu((QListViewItem*)static_QUType_ptr.get(_o+1)); break;
    case 4: clear(); break;
    default:
	return Page::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool ListBrowser::qt_emit( int _id, QUObject* _o )
{
    return Page::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool ListBrowser::qt_property( int id, int f, QVariant* v)
{
    return Page::qt_property( id, f, v);
}

bool ListBrowser::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
