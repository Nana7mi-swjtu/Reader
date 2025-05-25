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

// �ṹ�����ڴ洢 manifest �е���Ŀ��Ϣ
struct epubManifestItem
{
	QString id;
	QString href;
	QString mediaType;
	QString fallback;
};
//�ṹ�������洢 spine�е�item��Ϣ
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
	//��ȡspineItem
	QList<SpineItem> getSpineItem() const;
	//��ȡncx��manife�е�id
	QString getNcxItemId() const;


private:
	QuaZip* zEpubFile;
	QString zEpubFilePath;//epub�ļ�·��
	QString zOpfFilePath;//.opf��zip�е�·��
	QString zOpfbasePath;//.opf�ļ�����Ŀ¼��·��

	QMap<QString, epubManifestItem> zManifestItem; // ��->ֵ��id->item
	QList<SpineItem> zSpineItem;// �Ķ�˳�� (item ID �б�)
	QString zNcxItemId;//�洢spine�е�ncx����

	QMap<QString, QString>zNcxHrefToTitle;//href��title��ӳ��

	QVariantMap zMetadata;// �洢��������Ԫ����

	QString zLastError;

	// �ڲ���������
	bool parseContainerXml();
	QString findOpfFilePath(QuaZipFile& containerFileStream);
	bool parseOpfFile();
	void parseOpfMetadata(QXmlStreamReader& xml);
	void parseManifest(QXmlStreamReader& xml);
	void parseSpine(QXmlStreamReader& xml);
	//����ncx�ļ�
	bool parseNcxFile(const QString& ncxFilePathInZip);
	void parseNcxNavPoint(QXmlStreamReader& xml);

	// �� ZIP �ж�ȡ�ļ�����
	QString readFileContentFromZip(const QString& filePathInZip);
	QByteArray readBinaryFileContentFromZip(const QString& filePathInZip);
	//�淶href·��
	QString normalHref(const QString& opfBase, const QString& relHref) const;
};