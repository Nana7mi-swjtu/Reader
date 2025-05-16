#include "readerform.h"

readerform::readerform(QObject *parent)
	: QObject(parent) ,zEpubFile(nullptr)
{}

readerform::~readerform()
{
	closeEpub();
}


void readerform::closeEpub()
{
	if (zEpubFile)//�������ر�
	{
		zEpubFile->close();
		delete zEpubFile;
		zEpubFile = nullptr;
	}
	//���뵱ǰepub��ص�ȫ�����
	zManifestItem.clear();
	zSpineItemIds.clear();
	zMetadata.clear();
	zOpfbasePath.clear();
	zOpfFilePath.clear();
	zEpubFilePath.clear();
}

bool readerform::openEpub(const QString& filePath)
{
	closeEpub();//�ȹرշ�ֹ����

	zEpubFile = new QuaZip(filePath);
	zEpubFilePath = filePath;

	if (!zEpubFile->open(QuaZip::mdUnzip))
	{
		zLastError = tr("��%1ʧ��,error:%2").arg(filePath).arg(zEpubFile->getZipError());// ��ʧ�ܱ��������Ϣ
		qWarning() << zLastError;
		delete zEpubFile;
		zEpubFile = nullptr;
		return false;
	}
		
	if (!parseContainerXml())
	{
		return false;
	}

	if (!parseOpfFile())
	{
		return false;
	}

	/*for (const auto& item : qAsConst(zManifestItem))
	{
		if(item.)
	}*/

	zLastError.clear();//�򿪳ɹ������������Ϣ
	return true;
}

QMap<QString, QString> readerform::getTableofContent() const//��ȡĿ¼
{
	QMap<QString, QString> toc;
	for (const QString& itemId : qAsConst(zSpineItemIds))//��itemid�б��ж�ȡ
	{
		if (zManifestItem.contains(itemId))//�����Ƿ����itemid
		{
			toc.insert(itemId, zManifestItem.value(itemId).href);
		}
	}
	return toc;
}

QString readerform::getContentById(const QString& itemId)
{
	if (!zManifestItem.contains(itemId))
	{
		zLastError = tr("��manifest���Ҳ���%1").arg(itemId);
		qWarning() << zLastError;
		return QString();
	}

	const epubManifestItem& item = zManifestItem.value(itemId);//idת��Ϊitem
	QString filePathInZip = zOpfbasePath + item.href;

	//�淶·��
	QUrl opfBaseUrl = QUrl::fromLocalFile(zOpfbasePath.isEmpty() ? "/" : "/" + zOpfbasePath);

	if (!zOpfbasePath.endsWith('/') && !zOpfbasePath.isEmpty()) 
		opfBaseUrl = QUrl::fromLocalFile("/" + zOpfbasePath + "/");

	QUrl resolvedUrl = opfBaseUrl.resolved(item.href);
	filePathInZip = resolvedUrl.path();
	if (filePathInZip.startsWith("/"))
	{
		filePathInZip = filePathInZip.mid(1);
	}

	return readBinaryFileContentFromZip(filePathInZip);
}

QString readerform::getCoverImagePath() const
{
	if (!zMetadata.contains("cover"))//����Ƿ���cover
	{
		QString coverId = zMetadata.value("cover").toString();//ת����id
		if (zManifestItem.contains(coverId))
		{
			const epubManifestItem& item = zManifestItem.value(coverId);//ת��Ϊitem
			if (item.mediaType.startsWith("image/"))//���·��,���淶·������Ҫ�����opf��·��
			{
				QUrl opfBaseUrl = QUrl::fromLocalFile(zOpfbasePath.isEmpty() ? "/" : "/" + zOpfbasePath);
				if (!zOpfbasePath.endsWith('/') && !zOpfbasePath.isEmpty())
					opfBaseUrl = QUrl::fromLocalFile("/" + zOpfbasePath + "/");
				QUrl resolvedUrl = opfBaseUrl.resolved(item.href);
				QString fileInZip = resolvedUrl.path();
				if (fileInZip.startsWith("/"))
				{
					fileInZip = fileInZip.mid(1);
				}
				return fileInZip;
			}
		}
	}
}

QVariantMap readerform::getMetaDate() const
{
	return zMetadata;
}

QString readerform::getLastError() const
{
	return zLastError;
}

bool readerform::parseContainerXml()
{
	if (!zEpubFile || !zEpubFile->isOpen())
	{
		zLastError = tr("�ļ�δ��");
		qWarning() << zLastError;
		return false;
	}

	const QString containerPath = "META-INF/container.xml";

	if (!zEpubFile->setCurrentFile(containerPath))
	{
		zLastError = tr("%1��ʧ").arg(containerPath);
		qWarning() << zLastError;
		return false;
	}

	QuaZipFile containerFileStream(zEpubFile);
	if (!containerFileStream.open(QIODevice::ReadOnly))
	{
		zLastError = tr("�޷���%1��error:%2").arg(containerPath).arg(containerFileStream.getZipError());
		qWarning() << zLastError;
		return false;
	}

	zOpfFilePath = findOpfFilePath(containerFileStream);//��ȡopf�ļ�·��
	containerFileStream.close();

	if (zOpfFilePath.isEmpty())
	{
		if (zLastError.isEmpty())
		{
			zLastError = tr("�޷��ҵ�opf·��");
		}
		qWarning() << zLastError;
		return false;
	}

	QFileInfo opfInfo(zOpfFilePath);
	zOpfbasePath = opfInfo.path();
	if (zOpfbasePath == "." || zOpfbasePath.isEmpty())//���ÿ�·��
	{
		zOpfbasePath = "";
	}
	else if (!zOpfbasePath.endsWith("/"))//�淶·��
	{
		zOpfbasePath += "/";
	}

	qDebug() << "opf�ļ�·��Ϊ" << zOpfFilePath;
	qDebug() << "opf�ļ���·��Ϊ" << zOpfbasePath;
	return true;
}

QString readerform::findOpfFilePath(QuaZipFile& containerFileStream)
{

}