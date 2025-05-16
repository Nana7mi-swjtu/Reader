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
	if (zEpubFile)//如果打开则关闭
	{
		zEpubFile->close();
		delete zEpubFile;
		zEpubFile = nullptr;
	}
	//将与当前epub相关的全部请空
	zManifestItem.clear();
	zSpineItemIds.clear();
	zMetadata.clear();
	zOpfbasePath.clear();
	zOpfFilePath.clear();
	zEpubFilePath.clear();
}

bool readerform::openEpub(const QString& filePath)
{
	closeEpub();//先关闭防止出错

	zEpubFile = new QuaZip(filePath);
	zEpubFilePath = filePath;

	if (!zEpubFile->open(QuaZip::mdUnzip))
	{
		zLastError = tr("打开%1失败,error:%2").arg(filePath).arg(zEpubFile->getZipError());// 打开失败保存错误信息
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

	zLastError.clear();//打开成功，清除错误信息
	return true;
}

QMap<QString, QString> readerform::getTableofContent() const//获取目录
{
	QMap<QString, QString> toc;
	for (const QString& itemId : qAsConst(zSpineItemIds))//从itemid列表中读取
	{
		if (zManifestItem.contains(itemId))//查找是否存在itemid
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
		zLastError = tr("在manifest中找不到%1").arg(itemId);
		qWarning() << zLastError;
		return QString();
	}

	const epubManifestItem& item = zManifestItem.value(itemId);//id转化为item
	QString filePathInZip = zOpfbasePath + item.href;

	//规范路径
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
	if (!zMetadata.contains("cover"))//检查是否有cover
	{
		QString coverId = zMetadata.value("cover").toString();//转化成id
		if (zManifestItem.contains(coverId))
		{
			const epubManifestItem& item = zManifestItem.value(coverId);//转化为item
			if (item.mediaType.startsWith("image/"))//检查路径,并规范路径，需要相对于opf基路径
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
		zLastError = tr("文件未打开");
		qWarning() << zLastError;
		return false;
	}

	const QString containerPath = "META-INF/container.xml";

	if (!zEpubFile->setCurrentFile(containerPath))
	{
		zLastError = tr("%1丢失").arg(containerPath);
		qWarning() << zLastError;
		return false;
	}

	QuaZipFile containerFileStream(zEpubFile);
	if (!containerFileStream.open(QIODevice::ReadOnly))
	{
		zLastError = tr("无法打开%1，error:%2").arg(containerPath).arg(containerFileStream.getZipError());
		qWarning() << zLastError;
		return false;
	}

	zOpfFilePath = findOpfFilePath(containerFileStream);//获取opf文件路径
	containerFileStream.close();

	if (zOpfFilePath.isEmpty())
	{
		if (zLastError.isEmpty())
		{
			zLastError = tr("无法找到opf路径");
		}
		qWarning() << zLastError;
		return false;
	}

	QFileInfo opfInfo(zOpfFilePath);
	zOpfbasePath = opfInfo.path();
	if (zOpfbasePath == "." || zOpfbasePath.isEmpty())//设置空路径
	{
		zOpfbasePath = "";
	}
	else if (!zOpfbasePath.endsWith("/"))//规范路径
	{
		zOpfbasePath += "/";
	}

	qDebug() << "opf文件路径为" << zOpfFilePath;
	qDebug() << "opf文件基路径为" << zOpfbasePath;
	return true;
}

QString readerform::findOpfFilePath(QuaZipFile& containerFileStream)
{

}