#include <Monoworks.hh>
#include <core/Application.hh>
#include "MonoEditor.hh"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

int main(int argc, char** argv)
{
	return Monoworks::EditorMain(argc, argv);
}

namespace Monoworks 
{
	[[nodiscard]] int EditorMain(int argc, char** argv) 
	{
		QCoreApplication app(argc, argv);

		CApplication* engine = new CApplication;
		CEngineBridge bridge(engine);


		SApplicationCreateInfos appInfos{};
		appInfos.Name = "MonoEditor";
		appInfos.RenderableExtent = { 670, 480 };

		engine->Init(&appInfos);

		QQmlApplicationEngine qmlEngine;
		qmlEngine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
		

		QObject* rootObject = qmlEngine.rootObjects().first();
		QQuickWindow* window = qobject_cast<QQuickWindow*>(rootObject); 

		if (window)
			bridge.SetWindow(window);

		int result = app.exec();

		engine->Shutdown();

		return 0;
	}
}
