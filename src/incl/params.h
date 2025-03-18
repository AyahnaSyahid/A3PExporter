#ifndef ParamsH
#define ParamsH

struct Params {
	QString CLSID,
			exportDir,
			konsumen,
			from,
			exportName,
			bahan,
			strPages,
			strQty,
			strInfo;
	bool autoCurve,
		 doubleSided;
};

struct Detected {
    QString CLSID,
            documentName,
            documentPath;
    int pageCount;
};

struct Exported {
    QString CLSID,
            documentName,
            exportName,
            exportPath;
};
#endif