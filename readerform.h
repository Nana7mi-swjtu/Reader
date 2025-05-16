#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "reader.h"
#include "QuaZip-Qt6-1.5/quazip/quazip.h"
#include "QuaZip-Qt6-1.5/quazip/quazipfile.h"
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QUrl>

class QuaZip;
class QuaZipFile;
class QXmlStreamReader;

// �ṹ�����ڴ洢 manifest �е���Ŀ��Ϣ
struct epubManifestItem
{
	QString id;
	QString href;
	QString mediaType;
};


class readerform  : public QObject
{
	Q_OBJECT

public:
	readerform(QObject *parent);
	~readerform();
	//��epub�ļ�
	bool openEpub(const QString &filePath);
	//�ر�epub�ļ�
	void closeEpub();
	//��ȡĿ¼��id->href
	QMap<QString, QString> getTableofContent() const;
	//��ȡ�½�,id->content
	QString getContentById(const QString& itemId);
	//��ȡͼƬ·��
	QString getCoverImagePath() const;
	//��ȡԪ����
	QVariantMap getMetaDate() const;
	//��ȡ������Ϣ
	QString getLastError() const;

private:
	QuaZip* zEpubFile;
	QString zEpubFilePath;//epub�ļ�·��
	QString zOpfFilePath;//.opf��zip�е�·��
	QString zOpfbasePath;//.opf�ļ�����Ŀ¼��·��

	QMap<QString, epubManifestItem> zManifestItem; // ��: item ID
	QStringList zSpineItemIds;// �Ķ�˳�� (item ID �б�)
	QVariantMap zMetadata;// �洢��������Ԫ����
	//QString zNavDocument;//epub3�����ĵ���href

	QString zLastError;

	// �ڲ���������
	bool parseContainerXml();
	QString findOpfFilePath(QuaZipFile& containerFileStream);
	bool parseOpfFile();
	void parseOpfMetadata(QXmlStreamReader& xml);
	void parseManifest(QXmlStreamReader& xml);
	void parseSpine(QXmlStreamReader& xml);

	// �� ZIP �ж�ȡ�ļ�����
	QString readFileContentFromZip(const QString& filePathInZip);
	QByteArray readBinaryFileContentFromZip(const QString& filePathInZip);

};