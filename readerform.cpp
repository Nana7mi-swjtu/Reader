#include "readerform.h"

readerform::readerform(QObject *parent)
	: QObject(parent) ,zEpubFile(nullptr)
{}

readerform::~readerform()
{
	closeEpub();
}


QString readerform::normalHref(const QString& opfBase, const QString& relHref) const
{
	QString pathOnly = relHref;
	int anchorPos = pathOnly.indexOf('#');
	if (anchorPos != -1)//去除锚点
	{
		pathOnly = pathOnly.left(anchorPos);
	}

	QUrl baseUrl;
	if (opfBase.isEmpty() || opfBase == ".")
	{
		baseUrl = QUrl("file:///");
	}
	else
	{
		baseUrl = QUrl("file:///" + (opfBase.endsWith('/') ? opfBase : opfBase + "/"));
	}

	QUrl resolvedUrl = baseUrl.resolved(QUrl::fromUserInput(pathOnly));
	QString filePath = resolvedUrl.path();
	if (filePath.startsWith('/'))
	{
		filePath = filePath.mid(1);
	}

	return filePath;
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
	zSpineItem.clear();
	zMetadata.clear();
	zOpfbasePath.clear();
	zOpfFilePath.clear();
	zEpubFilePath.clear();
	zLastError.clear();
	zNcxItemId.clear();
	zNcxHrefToTitle.clear();
}

bool readerform::openEpub(const QString& filePath)
{
	closeEpub();//先关闭防止出错

	zEpubFile = new QuaZip(filePath);
	zEpubFilePath = filePath;

	if (!zEpubFile->open(QuaZip::mdUnzip))
	{
		zLastError = tr("Failed to open Epub File %1, error: % 2").arg(filePath).arg(zEpubFile->getZipError());// 打开失败保存错误信息
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


	//解析ncx
	if (!zNcxItemId.isEmpty())
	{
		if (zManifestItem.contains(zNcxItemId))
		{
			const epubManifestItem& ncxItem = zManifestItem.value(zNcxItemId);
			QString  ncxFilePathInZip = normalHref(zOpfbasePath, ncxItem.href);

			qDebug() << "NCX file found in manifest via spine toc id" << zNcxItemId << ":" << ncxItem.href << "-> Full path in zip :" << ncxFilePathInZip;

			if (!parseNcxFile(ncxFilePathInZip))
			{
				qWarning() << "Failed to parse ncx File , table of content will use fallback" << getLastError();
				zLastError.clear();
			}
		}
		else
		{
			qWarning() << "NCX item ID" << zNcxItemId << "not found in manifest";
		}
	}
	else
	{
		qWarning() << "no NCX item ID";
	}

	zLastError.clear();//打开成功，清除错误信息
	return true;
}

QMap<QString, QString> readerform::getTableofContent() const
{
	QMap<QString, QString> tocDisplayMap;
	for (const SpineItem& spineItem : std::as_const(zSpineItem))
	{
		if (!spineItem.linear)
		{
			continue;
		}

		if (zManifestItem.contains(spineItem.idref))
		{
			const epubManifestItem& manifestItem = zManifestItem.value(spineItem.idref);
			QString manifestHref = manifestItem.href;
			QString normalManifestHref = normalHref(zOpfbasePath, manifestHref);

			QString displayString;
			if (zNcxHrefToTitle.contains(normalManifestHref))
			{
				displayString = zNcxHrefToTitle.value(normalManifestHref);
			}
			else//如果没有此条目，则使用文件名
			{
				QFileInfo fileInfo(manifestHref);
				displayString = fileInfo.fileName();
				if (displayString.isEmpty())//没有文件名，则使用id
				{
					displayString = manifestItem.id;
				}
			}
			tocDisplayMap.insert(spineItem.idref, displayString);
		}
		else
		{
			qWarning() << "Spine item idref " << spineItem.idref << " not found in manifest.";
			tocDisplayMap.insert(spineItem.idref, spineItem.idref + " not found.");
		}
	}
	return tocDisplayMap;
}

QString readerform::getContentById(const QString& itemId)
{
	if (!zManifestItem.contains(itemId))
	{
		zLastError = tr("Content item with id(%1) not found in manifest").arg(itemId);
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

QList<SpineItem> readerform::getSpineItem() const
{
	return zSpineItem;
}

QString readerform::getNcxItemId() const
{
	return zNcxItemId;
}

bool readerform::parseContainerXml()
{
	if (!zEpubFile || !zEpubFile->isOpen())//未打开
	{
		zLastError = tr("Epub file not open");
		qWarning() << zLastError;
		return false;
	}

	const QString containerPath = "META-INF/container.xml";//存储路径

	if (!zEpubFile->setCurrentFile(containerPath))//设置失败
	{
		zLastError = tr("Epub missing '%1'").arg(containerPath);
		qWarning() << zLastError;
		return false;
	}

	QuaZipFile containerFileStream(zEpubFile);
	if (!containerFileStream.open(QIODevice::ReadOnly))
	{
		zLastError = tr("not open %1，error:%2").arg(containerPath).arg(containerFileStream.getZipError());
		qWarning() << zLastError;
		return false;
	}

	zOpfFilePath = findOpfFilePath(containerFileStream);//获取opf文件路径
	containerFileStream.close();

	if (zOpfFilePath.isEmpty())//路径为空
	{
		if (zLastError.isEmpty())
		{
			zLastError = tr("not find opf file path");
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

	qDebug() << "opf file path in zip " << zOpfFilePath;
	qDebug() << "opf file base path in zip " << zOpfbasePath;
	return true;
}

QString readerform::findOpfFilePath(QuaZipFile& containerFileStream)
{
	QXmlStreamReader xml(&containerFileStream);//解析epubxml
	while (!xml.atEnd() && !xml.hasError())
	{
		xml.readNext();
		if (xml.isStartElement() && xml.name().toString() == "rootfile")
		{
			QXmlStreamAttributes attributes = xml.attributes();//提取属性
			if (attributes.hasAttribute("full-path"))
			{
				return attributes.value("full-path").toString();
			}
		}
	}

	if (xml.hasError())//出现错误发送错误信息
	{
		zLastError = tr("xml parsing error：%1").arg(xml.errorString());
		qWarning() << zLastError;
	}

	return QString();//返回空路径
}

bool readerform::parseOpfFile()
{
	if (!zEpubFile || !zEpubFile->isOpen() || zOpfFilePath.isEmpty())
	{
		zLastError = tr("opf file path is empty or epub is not open");
		qWarning() << zLastError;
		return false;
	}

	if (!zEpubFile->setCurrentFile(zOpfFilePath))//设置路径
	{
		zLastError = tr("not find opf file：%1，error：%2").arg(zOpfFilePath).arg(zEpubFile->getZipError());
		qWarning() << zLastError;
		return false;
	}

	QuaZipFile opfContentFile(zEpubFile);//打开epub文件
	if (!opfContentFile.open(QIODevice::ReadOnly))
	{
		zLastError = tr("open opf file failed：%1，error：%2").arg(zOpfFilePath).arg(opfContentFile.getZipError());
		qWarning() << zLastError;
		return false;
	}

	QXmlStreamReader xml(&opfContentFile);//解析xml格式
	while (!xml.atEnd() && !xml.hasError())
	{
		xml.readNext();
		if (xml.isStartElement())
		{
			if (xml.name().toString() == "metadata")
			{
				parseOpfMetadata(xml);
			}
			if (xml.name().toString() == "manifest")
			{
				parseManifest(xml);
			}
			if (xml.name().toString() == "spine")
			{
				parseSpine(xml);
			}
		}
	}

	opfContentFile.close();

	if (xml.hasError())
	{
		zLastError = tr("xml parsing failed in %1：%2").arg(zOpfFilePath).arg(xml.errorString());
		qWarning() << zLastError;
		return false;
	}

}

void readerform::parseOpfMetadata(QXmlStreamReader& xml)
{
	if (!xml.isStartElement() || xml.name().toString() == "metadata")//防止错误进入
		return;

	const QString dcNamespaceUri = "http://purl.org/dc/elements/1.1/";

	// 用于收集可重复的元数据项
	QList<QVariantMap> creatorsList;
	QList<QVariantMap> contributorsList;
	QList<QVariantMap> identifiersList;
	QList<QVariantMap> datesList;
	QStringList subjectsList;
	QStringList languagesList;
	QStringList typesList;       
	QStringList formatsList;     
	QStringList sourcesList;    
	QStringList relationsList;   
	QStringList coveragesList;  

	// 临时存储父dc元素的ID，以备子<meta>元素通过opf:refines引用
	// QString currentDCOpjectId; // 如果要支持opf:refines，但这里简化，暂不处理

	while (!(xml.isEndElement() && xml.name().toString() == "metadata") && !xml.atEnd()) 
	{
		xml.readNext();

		if (xml.isStartElement()) 
		{
			QString tagName = xml.name().toString(); // 本地名
			QString nsUri = xml.namespaceUri().toString(); // 命名空间 URI
			QXmlStreamAttributes attributes = xml.attributes();

			// 处理 <meta> 标签 
			if (tagName == "meta") 
			{
				if (attributes.hasAttribute("name") && attributes.value("name") == "cover" && attributes.hasAttribute("content")) 
				{
					// EPUB 2 Cover ID (来自 <meta name="cover" content="cover-image-id"/>)
					zMetadata["cover_ref_id"] = attributes.value("content").toString();
				}
				
				if (xml.isStartElement()) // 确保仍然是开始标签，以防万一
				{ 
					xml.skipCurrentElement(); // 跳过meta标签的任何潜在内容和结束标签
				}
				continue; // 处理完meta标签，进入下一次循环
			}
			// 处理dc元素 
			else if (nsUri == dcNamespaceUri) 
			{
				QString elementId = attributes.value("id").toString(); // 获取DC元素的id属性
				// 使用 readElementText 获取元素的所有文本内容，并跳过任何子元素
				QString textContent = xml.readElementText(QXmlStreamReader::SkipChildElements).trimmed();

				if (tagName == "title") 
				{
					QVariantMap titleMap;
					titleMap["value"] = textContent;
					if (!elementId.isEmpty()) 
						titleMap["id"] = elementId;
					// EPUB 2 title 通常不复杂，但可以有id。主要标题通常只有一个。
					// 如果有多个<dc:title>，这里会取最后一个。可改为列表或按类型区分。
					zMetadata["title"] = textContent; // 简化：只存文本
					if (!elementId.isEmpty()) 
						zMetadata["title_id"] = elementId; // 存ID
				}
				else if (tagName == "creator") 
				{
					QVariantMap entryMap;
					entryMap["name"] = textContent;
					if (!elementId.isEmpty()) 
						entryMap["id"] = elementId;
					if (attributes.hasAttribute("role")) 
						entryMap["role"] = attributes.value("role").toString(); // opf:role
					if (attributes.hasAttribute("file-as")) 
						entryMap["file-as"] = attributes.value("file-as").toString(); // opf:file-as
					creatorsList.append(entryMap);
				}
				else if (tagName == "contributor") 
				{
					QVariantMap entryMap;
					entryMap["name"] = textContent;
					if (!elementId.isEmpty()) 
						entryMap["id"] = elementId;
					if (attributes.hasAttribute("role")) 
						entryMap["role"] = attributes.value("role").toString();
					if (attributes.hasAttribute("file-as")) 
						entryMap["file-as"] = attributes.value("file-as").toString();
					contributorsList.append(entryMap);
				}
				else if (tagName == "identifier") 
				{
					QVariantMap entryMap;
					entryMap["value"] = textContent;
					if (!elementId.isEmpty()) 
						entryMap["id"] = elementId;
					// opf:scheme 在 EPUB 2 中是可选的，但常见
					if (attributes.hasAttribute("scheme")) 
						entryMap["scheme"] = attributes.value("scheme").toString(); // opf:scheme
					identifiersList.append(entryMap);
				}
				else if (tagName == "language") 
				{
					languagesList.append(textContent);
				}
				else if (tagName == "subject") 
				{
					subjectsList.append(textContent);
				}
				else if (tagName == "description") 
				{
					// 通常一个主要描述
					zMetadata["description"] = textContent;
					if (!elementId.isEmpty()) 
						zMetadata["description_id"] = elementId;
				}
				else if (tagName == "publisher") 
				{
					zMetadata["publisher"] = textContent;
					if (!elementId.isEmpty()) zMetadata["publisher_id"] = elementId;
				}
				else if (tagName == "date") 
				{
					QVariantMap entryMap;
					entryMap["value"] = textContent;
					if (!elementId.isEmpty()) 
						entryMap["id"] = elementId;
					if (attributes.hasAttribute("event")) 
						entryMap["event"] = attributes.value("event").toString(); // opf:event
					datesList.append(entryMap);
				}
				else if (tagName == "rights") 
				{
					zMetadata["rights"] = textContent;
					if (!elementId.isEmpty()) 
						zMetadata["rights_id"] = elementId;
				}
				else if (tagName == "source") 
				{
					sourcesList.append(textContent);
				}
				else if (tagName == "relation") 
				{
					relationsList.append(textContent);
				}
				else if (tagName == "coverage") 
				{
					coveragesList.append(textContent);
				}
				else if (tagName == "type") 
				{
					typesList.append(textContent);
				}
				else if (tagName == "format") 
				{
					formatsList.append(textContent);
				}
				// xml.readElementText() 已经消耗了元素的结束标签
				// 所以循环的下一次迭代将从该元素的下一个兄弟元素开始。
			}
		}
	} // 结束 while 循环 (metadata 块解析完毕)

	// 将收集到的列表存储到 zMetadata 中
	if (!creatorsList.isEmpty()) 
		zMetadata["authors"] = QVariant::fromValue(creatorsList);
	if (!contributorsList.isEmpty()) 
		zMetadata["contributors"] = QVariant::fromValue(contributorsList);
	if (!identifiersList.isEmpty()) 
		zMetadata["identifiers"] = QVariant::fromValue(identifiersList);
	if (!datesList.isEmpty()) 
		zMetadata["dates"] = QVariant::fromValue(datesList);
	if (!subjectsList.isEmpty()) 
		zMetadata["subjects"] = QVariant::fromValue(subjectsList);

	// 对于单个或多个语言、类型等，进行相应存储
	if (!languagesList.isEmpty()) 
	{
		zMetadata["language"] = (languagesList.size() == 1) ? QVariant(languagesList.first()) : QVariant::fromValue(languagesList);
	}
	if (!typesList.isEmpty()) 
		zMetadata["type"] = (typesList.size() == 1) ? QVariant(typesList.first()) : QVariant::fromValue(typesList);
	if (!formatsList.isEmpty()) 
		zMetadata["format"] = (formatsList.size() == 1) ? QVariant(formatsList.first()) : QVariant::fromValue(formatsList);
	if (!sourcesList.isEmpty()) 
		zMetadata["source"] = (sourcesList.size() == 1) ? QVariant(sourcesList.first()) : QVariant::fromValue(sourcesList);
	if (!relationsList.isEmpty()) 
		zMetadata["relation"] = (relationsList.size() == 1) ? QVariant(relationsList.first()) : QVariant::fromValue(relationsList);
	if (!coveragesList.isEmpty()) 
		zMetadata["coverage"] = (coveragesList.size() == 1) ? QVariant(coveragesList.first()) : QVariant::fromValue(coveragesList);

}

void readerform::parseManifest(QXmlStreamReader& xml)
{
	if (!xml.isStartElement() || xml.name().toString() == "manifest")//防止无manifest标签
	{
		zLastError = tr("no manifest tag");
		qWarning() << zLastError;
		return;
	}

	while (!(xml.isEndElement() && xml.name().toString() == "manifest") && !xml.atEnd())
	{
		xml.readNext();
		if (xml.isStartElement() && xml.name().toString() == "item")
		{
			QXmlStreamAttributes attributs = xml.attributes();//获取属性
			epubManifestItem currentItem;

			if (attributs.hasAttribute("id"))
			{
				currentItem.id = attributs.value("id").toString();
			}
			else
			{
				qWarning() << " missing id";
				xml.skipCurrentElement();//无id跳过元素
				continue;
			}

			if (attributs.hasAttribute("href"))
			{
				currentItem.href = attributs.value("href").toString();
			}
			else
			{
				qWarning() << "missing href";
				xml.skipCurrentElement();//无href跳过元素
				continue;
			}

			if (attributs.hasAttribute("media-type"))
			{
				currentItem.mediaType = attributs.value("media-type").toString();
			}
			else
			{
				qWarning() << "missing media-type";
				xml.skipCurrentElement();//无media-type跳过元素
				continue;
			}

			if (attributs.hasAttribute("fallback"))
			{
				currentItem.fallback = attributs.value("fallback").toString();
			}

			if (!currentItem.id.isEmpty() && !currentItem.href.isEmpty() && !currentItem.mediaType.isEmpty())//都不缺失则插入到目录
			{
				zManifestItem.insert(currentItem.id, currentItem);
			}

		}
	}
	if (xml.hasError())
	{
		zLastError = tr("xml error:%1").arg(xml.errorString());
	}
}

void readerform::parseSpine(QXmlStreamReader& xml)
{
	if (!xml.isStartElement() || xml.name().toString() != "spine")
	{
		zLastError = tr("no spine tag");
		qWarning() << zLastError;
		return;
	}

	QXmlStreamAttributes spineAttributes = xml.attributes();//获取spine属性

	if (spineAttributes.hasAttribute("toc"))//提取toc属性
	{
		zNcxItemId = spineAttributes.value("toc").toString();
	}
	else
	{
		zLastError = tr("no toc attribute");
		qWarning() << zLastError;
		return;
	}

	zSpineItem.clear();
	while (!(xml.isEndElement() && xml.name().toString() == "spine") && !xml.atEnd())
	{
		xml.readNext();
		if (xml.isStartElement() && xml.name().toString() == "itemref")
		{
			SpineItem currentSpineItem;
			QXmlStreamAttributes itemrefAttribute = xml.attributes();
			//提取idref属性
			if (itemrefAttribute.hasAttribute("idref"))
			{
				currentSpineItem.idref = itemrefAttribute.value("idref").toString();
			}
			else
			{
				zLastError = tr("no idref attribute");
				qWarning() << zLastError;
				xml.skipCurrentElement();//跳过无效属性
				continue;
			}

			if (itemrefAttribute.hasAttribute("linear"))
			{
				if (itemrefAttribute.value("linear").toString() == "no")
				{
					currentSpineItem.linear = false;
				}
				else
				{
					currentSpineItem.linear = true;
				}
			}
			else
			{
				currentSpineItem.linear = true;
			}
			//加入spineItem列表
			if (!currentSpineItem.idref.isEmpty())
			{
				zSpineItem.append(currentSpineItem);
			}
		}

	}

	if (xml.hasError())
	{
		zLastError = tr("xml error:%1").arg(xml.errorString());
		qWarning() << zLastError;
	}
}

QString readerform::readFileContentFromZip(const QString& filePathInZip)
{
	if (!zEpubFile || !zEpubFile->isOpen())
	{
		zLastError = tr("epub file is not open when read %1").arg(filePathInZip);
		qWarning() << zLastError;
		return QString();
	}

	if (!zEpubFile->setCurrentFile(filePathInZip))
	{
		if (!zEpubFile->setCurrentFile(filePathInZip, QuaZip::csInsensitive))
		{
			zLastError = tr("could not set current file %1，error：%2").arg(filePathInZip).arg(zEpubFile->getZipError());
			qWarning() << zLastError;
			return QString();
		}
	}

	QuaZipFile fileEntry(zEpubFile);
	if (!fileEntry.open(QIODevice::ReadOnly))
	{
		zLastError = tr("could not open file：%1，error：%2").arg(filePathInZip).arg(fileEntry.getZipError());
		qWarning() << zLastError;
		return QString();
	}

	QByteArray data = fileEntry.readAll();
	fileEntry.close();
	return QString::fromUtf8(data);
}

QByteArray readerform::readBinaryFileContentFromZip(const QString& filePathInZip)
{
	if (!zEpubFile || !zEpubFile->isOpen())
	{
		zLastError = tr("epub file is not open when read %1").arg(filePathInZip);
		qWarning() << zLastError;
		return QByteArray();
	}

	if (!zEpubFile->setCurrentFile(filePathInZip))
	{
		if (!zEpubFile->setCurrentFile(filePathInZip, QuaZip::csInsensitive))
		{
			zLastError = tr("could not set current file %1，error：%2").arg(filePathInZip).arg(zEpubFile->getZipError());
			qWarning() << zLastError;
			return QByteArray();
		}
	}

	QuaZipFile fileEntry(zEpubFile);
	if (!fileEntry.open(QIODevice::ReadOnly))
	{
		zLastError = tr("could not open file：%1，error：%2").arg(filePathInZip).arg(fileEntry.getZipError());
		qWarning() << zLastError;
		return QByteArray();
	}

	QByteArray data = fileEntry.readAll();
	fileEntry.close();
	return data;
}

bool readerform::parseNcxFile(const QString& ncxFilePathInZip)
{
	zNcxHrefToTitle.clear();
	QString ncxContnet = readFileContentFromZip(ncxFilePathInZip);

	if (ncxContnet.isEmpty())
	{
		zLastError = tr("NCX file content is empty:%1").arg(ncxFilePathInZip);
		return false;
	}

	QXmlStreamReader xml(ncxContnet);

	while (!xml.atEnd() && !xml.hasError())
	{
		xml.readNext();

		if (xml.isStartElement())
		{
			if (xml.name().toString() == "nacMap")
			{
				while (!(xml.isEndElement() && xml.name().toString() == "navMap") && !xml.atEnd())//调用辅助解析函数
				{
					xml.readNext();
					if (xml.isStartElement() && xml.name().toString() == "navPoint")
					{
						parseNcxNavPoint(xml);
					}
				}
			}
		}
	}

	if (xml.hasError())
	{
		zLastError = tr("xml error in Ncx file(%1):%2").arg(ncxFilePathInZip).arg(xml.errorString());
		return false;
	}

	return true;
}

void readerform::parseNcxNavPoint(QXmlStreamReader& xml)
{
	if (!xml.isStartElement() || xml.name().toString() == "navPoint")
	{
		return;
	}

	QString currentTitle;
	QString	currentCotentSrc;

	while (!(xml.isEndElement() && xml.name().toString() == "navPoint") && !xml.atEnd())
	{
		xml.readNext();
		if (xml.isStartElement())
		{
			QString tagName = xml.name().toString();

			if (tagName == "navLabel")
			{
				while (!(xml.isEndElement() && xml.name().toString() == "navLabel"))
				{
					xml.readNext();

					if (xml.isStartElement() && xml.name().toString() == "text")
					{
						currentTitle = xml.readElementText().trimmed();
						break;
					}
				}
			}

			else if (tagName == "content")
			{
				currentCotentSrc = xml.attributes().value("scr").toString();

				if (xml.isStartElement())
				{
					xml.skipCurrentElement();
				}
			}

			else if (tagName == "navPoint")
			{
				parseNcxNavPoint(xml);
			}
		}
	}

	if (!currentTitle.isEmpty() && !currentCotentSrc.isEmpty())
	{
		QString normalScr = normalHref(zOpfbasePath, currentCotentSrc);

		if (!normalScr.isEmpty())
		{
			zNcxHrefToTitle[normalScr] = currentTitle;
		}
	}
}