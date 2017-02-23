ROOT = ../../../../..

include ${ROOT}/rules/defs.mk
LIBBASES = avl bioioC commonC fastCMaths hashTableC hashTableC_itr \
	pairwiseAlignment sonLibCache sonLibCommon sonLibCompression \
	sonLibConnectivity sonLibEulerTour sonLibExcept sonLibFile sonLibHash \
	sonLibKVDatabase_BigRecordFile sonLibKVDatabase sonLibKVDatabaseConf \
	sonLibKVDatabase_MySql sonLibKVDatabase_TokyoCabinet sonLibList \
	sonLibNaiveConnectivity sonLibRandom sonLibSet sonLibSortedSet sonLibString \
	sonLibTreap sonLibTree sonLibTuples stGraph stMatrix stPhylogeny \
	stPosetAlignment stSafeC stThreadPool stUnionFind
LIBFILE = ${sonlib_lib}
CFLAGS_INCL += -I${sonlib_incldir} -I../../lz4 -I../../quicktree/include



include ${ROOT}/rules/rules.mk
