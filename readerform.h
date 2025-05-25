#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "QuaZip-Qt6-1.5/quazip/quazip.h"
#include "QuaZip-Qt6-1.5/quazip/quazipfile.h"
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QDir>

class QuaZip;
class QuaZipFile;
class QXmlStreamReader;

// 结构体用于存储 manifest 中的项目信息
struct epubManifestItem
{
	QString id;
	QString href;
	QString mediaType;
	QString fallback;
};
//结构体用来存储 spine中的item信息
struct SpineItem
{
	QString idref;
	bool linear = true;
};


class readerform  : public QObject
{
	Q_OBJECT

public:
	readerform(QObject *parent);
	~readerform();
	//打开epub文件
	bool openEpub(const QString &filePath);
	//关闭epub文件
	void closeEpub();
	//获取目录，id->href
	QMap<QString, QString> getTableofContent() const;
	//获取章节,id->content
	QString getContentById(const QString& itemId);
	//获取图片路径
	QString getCoverImagePath() const;
	//获取元数据
	QVariantMap getMetaDate() const;
	//获取错误信息
	QString getLastError() const;
	//获取spineItem
	QList<SpineItem> getSpineItem() const;
	//获取ncx在manife中的id
	QString getNcxItemId() const;


private:
	QuaZip* zEpubFile;
	QString zEpubFilePath;//epub文件路径
	QString zOpfFilePath;//.opf在zip中的路径
	QString zOpfbasePath;//.opf文件所在目录的路径

	QMap<QString, epubManifestItem> zManifestItem; // 键->值：id->item
	QList<SpineItem> zSpineItem;// 阅读顺序 (item ID 列表)
	QString zNcxItemId;//存储spine中的ncx属性

	QMap<QString, QString>zNcxHrefToTitle;//href到title的映射

	QVariantMap zMetadata;// 存储解析到的元数据

	QString zLastError;

	// 内部解析函数
	bool parseContainerXml();
	QString findOpfFilePath(QuaZipFile& containerFileStream);
	bool parseOpfFile();
	void parseOpfMetadata(QXmlStreamReader& xml);
	void parseManifest(QXmlStreamReader& xml);
	void parseSpine(QXmlStreamReader& xml);
	//解析ncx文件
	bool parseNcxFile(const QString& ncxFilePathInZip);
	void parseNcxNavPoint(QXmlStreamReader& xml);

	// 从 ZIP 中读取文件内容
	QString readFileContentFromZip(const QString& filePathInZip);
	QByteArray readBinaryFileContentFromZip(const QString& filePathInZip);
	//规范href路径
	QString normalHref(const QString& opfBase, const QString& relHref) const;
};