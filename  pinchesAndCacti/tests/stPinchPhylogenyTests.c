#include <stdlib.h>
#include "CuTest.h"
#include "sonLib.h"
#include "stPinchPhylogeny.h"

static char getRandomNucleotide() {
    double d = st_random();
    return d >= 0.75 ? 'A' : (d >= 0.5 ? 'T' : (d >= 0.25 ? 'G' : 'C'));
}

char *getRandomDNAString(int64_t length) {
    char *string = st_malloc(sizeof(char)*(length+1));
    for(int64_t i=0; i<length; i++) {
        string[i] = getRandomNucleotide();
    }
    string[length] = '\0';
    return string;
}

/*
 * For each thread in graph creates a DNA string and places it in a hash of threads to strings.
 */
static stHash *getRandomStringsForGraph(stPinchThreadSet *randomGraph) {
    stHash *randomStrings = stHash_construct2(NULL, free);
    stPinchThreadSetIt threadIt = stPinchThreadSet_getIt(randomGraph);
    stPinchThread *thread;
    while((thread = stPinchThreadSetIt_getNext(&threadIt)) != NULL) {
        stHash_insert(randomStrings, thread, getRandomDNAString(stPinchThread_getLength(thread)));
    }
    return randomStrings;
}

/*
 * Get the pinch end adjacent to the given side of the feature segment.
 */
stPinchEnd getAdjacentPinchEnd(stFeatureSegment *featureSegment, bool traverse5Prime) {
    stPinchSegment *segment = featureSegment->segment;
    do {
        segment = traverse5Prime ? stPinchSegment_get5Prime(segment) : stPinchSegment_get3Prime(segment);
        if(segment == NULL) {
            return stPinchEnd_constructStatic(NULL, 0);
        }
    } while(stPinchSegment_getBlock(segment) == NULL);
    return stPinchEnd_constructStatic(stPinchSegment_getBlock(segment), !traverse5Prime);
}

static void getDistances(stPinchSegment *segment, stPinchSegment *segment2, bool ignoreUnalignedBases,
        int64_t *distance, int64_t *blockDistance) {
    *distance = 0;
    *blockDistance = 0;
    while(segment != segment2) {
        if(!ignoreUnalignedBases || stPinchSegment_getBlock(segment2) != NULL) {
            *distance += stPinchSegment_getLength(segment);
            (*blockDistance)++;
        }
        segment = stPinchSegment_get3Prime(segment);
    }
    *distance += stPinchSegment_getLength(segment2)/2;
}

/*
 * Get the distance from segment to the mid point of segment2, in terms of bases.
 */
static int64_t getDistance(stPinchSegment *segment, stPinchSegment *segment2, bool ignoreUnalignedBases) {
    int64_t distance, blockDistance;
    getDistances(segment, segment2, ignoreUnalignedBases,
            &distance, &blockDistance);
    return distance;
}

/*
 * Get the block distance from segment to segment2.
 */
static int64_t getBlockDistance(stPinchSegment *segment, stPinchSegment *segment2, bool ignoreUnalignedBases) {
    int64_t distance, blockDistance;
    getDistances(segment, segment2, ignoreUnalignedBases,
            &distance, &blockDistance);
    return blockDistance;
}

/*
 * Walk from the segment to the segment contained in the block of interest, using distance.
 */
stFeatureSegment *traceToMidFeatureSegmentUsingDistance(stFeatureSegment *featureSegment) {

}

/*
 * Gets the index of the segment in the list of segments associated with a block, in the order provided by the iterator.
 */
int64_t indexOfSegment(stPinchSegment *segment) {
    stPinchBlockIt segmentIt = stPinchBlock_getSegmentIterator(stPinchSegment_getBlock(segment));
    stPinchSegment *segment2;
    int64_t i=0;
    while((segment2 = stPinchBlockIt_getNext(&segmentIt)) != NULL) {
        if(segment2 == segment) {
            return i;
        }
        i++;
    }
    return INT64_MAX;
}

/*
 * Tests that from random pinch graphs we get reliable set of contextual feature blocks,
 * and applies the assessFeatureBlocksFn to the block.
 */
static void makeAndTestRandomFeatureBlocks(CuTest *testCase,
        void (*assessFeatureBlocksFn)(stPinchBlock *, stList *, CuTest *)) {
    for (int64_t test = 0; test < 100; test++) {
        //Make random graph
        stPinchThreadSet *randomGraph = stPinchThreadSet_getRandomGraph();
        stHash *randomStrings = getRandomStringsForGraph(randomGraph);

        //Make random thread set
        stPinchThreadSetBlockIt blockIt = stPinchThreadSet_getBlockIt(randomGraph);
        stPinchBlock *block;

        while ((block = stPinchThreadSetBlockIt_getNext(&blockIt)) != NULL) {
            assert(stPinchBlock_getDegree(block) > 1);
            /*
             * Choose random parameters.
             */
            int64_t maxBaseDistance = st_randomInt(0, 10000);
            int64_t maxBlockDistance = st_randomInt(0, 100);
            bool ignoreUnalignedBases = st_random() > 0.5;
            bool onlyIncludeCompleteFeatureBlocks = st_random() > 0.5;

            //Get the feature blocks
            stList *featureBlocks = stFeatureBlock_getContextualFeatureBlocks(block, maxBaseDistance, maxBlockDistance,
                    ignoreUnalignedBases, onlyIncludeCompleteFeatureBlocks, randomStrings);

            /*
             * Check properties of feature blocks.
             */

            //There must be one or more feature blocks.
            CuAssertTrue(testCase, stList_length(featureBlocks) >= 1);

            //Make hash of pinch segments to feature segments, so we can validate we get the correct list.
            stHash *pinchSegmentsToFeatureSegments = stHash_construct();

            //Check that each feature block corresponds to a given block, and contains the correct segments.
            //and that they have the correct properties.
            for (int64_t i = 0; i < stList_length(featureBlocks); i++) {
                stFeatureBlock *featureBlock = stList_get(featureBlocks, i);

                //Check the head is not null
                CuAssertTrue(testCase, featureBlock->head != NULL);

                //Now iterate through feature segments
                stFeatureSegment *featureSegment = featureBlock->head;
                stPinchBlock *block2 = stPinchSegment_getBlock(featureBlock->head->segment);
                while (featureSegment != NULL) {
                    //Add to the hash which we'll use to check for presence
                    stHash_insert(pinchSegmentsToFeatureSegments, featureSegment->segment, featureSegment);

                    //Check pointer to pinch segment and is from the correct block.
                    CuAssertPtrEquals(testCase, block2, stPinchSegment_getBlock(featureSegment->segment));

                    //Check lengths
                    CuAssertIntEquals(testCase, featureBlock->length, featureSegment->length);
                    CuAssertIntEquals(testCase, featureSegment->length, stPinchSegment_getLength(featureSegment->segment));

                    //Check orientation is as expected
                    CuAssertTrue(testCase,
                            featureSegment->reverseComplement
                                    != stPinchSegment_getBlockOrientation(featureSegment->segment));

                    //Check string is as expected
                    const char *threadString = stHash_search(randomStrings,
                            stPinchSegment_getThread(featureSegment->segment));
                    for (int64_t j = 0; j < featureSegment->length; j++) {
                        CuAssertIntEquals(testCase,
                                threadString[stPinchSegment_getStart(featureSegment->segment) + j
                                        - stPinchThread_getStart(stPinchSegment_getThread(featureSegment->segment))],
                                featureSegment->string[j]);
                    }

                    //Check pinch ends
                    if (featureSegment->reverseComplement) {
                        CuAssertPtrEquals(testCase,
                                featureSegment->nPinchEnd.block,
                                        getAdjacentPinchEnd(featureSegment, featureSegment->reverseComplement).block);
                        CuAssertTrue(testCase,featureSegment->nPinchEnd.orientation
                                     == getAdjacentPinchEnd(featureSegment, featureSegment->reverseComplement).orientation);
                        CuAssertPtrEquals(testCase,
                                featureSegment->pPinchEnd.block,
                                        getAdjacentPinchEnd(featureSegment, !featureSegment->reverseComplement).block);
                        CuAssertTrue(testCase, featureSegment->pPinchEnd.orientation
                                                                == getAdjacentPinchEnd(featureSegment, !featureSegment->reverseComplement).orientation);
                    }

                    //Check distance and index
                    stFeatureSegment *midFeatureSegment = traceToMidFeatureSegmentUsingDistance(featureSegment);
                    CuAssertIntEquals(testCase, featureSegment->segmentIndex, midFeatureSegment->segmentIndex);
                    CuAssertIntEquals(testCase, featureSegment->segmentIndex,
                            indexOfSegment(midFeatureSegment->segment));
                    if(featureSegment->distance) {
                        CuAssertIntEquals(testCase, getDistance(featureSegment->segment, midFeatureSegment->segment, ignoreUnalignedBases), featureSegment->distance);
                    }
                    else {
                        CuAssertIntEquals(testCase, getDistance(midFeatureSegment->segment, featureSegment->segment, ignoreUnalignedBases), featureSegment->distance);
                    }

                    //Check tail pointer is correct
                    if (featureSegment->nFeatureSegment == NULL) {
                        CuAssertPtrEquals(testCase, featureBlock->tail, featureSegment);
                    }
                    featureSegment = featureSegment->nFeatureSegment;
                }
            }

            //Check that there are no segments that are not in feature blocks.
            stPinchBlockIt segmentIt = stPinchBlock_getSegmentIterator(block);
            stPinchSegment *segment;
            while ((segment = stPinchBlockIt_getNext(&segmentIt)) != NULL) {
                //Walk 5' until we find a segment that does not have a feature segment, then test to check it properly shouldn't be a feature segment for the given
                //block segment
                stPinchSegment *segment2 = segment;
                while (segment2 != NULL
                        && (stPinchSegment_getBlock(segment2) == NULL
                                || stHash_search(pinchSegmentsToFeatureSegments, segment2) != NULL)) {
                    segment2 = stPinchSegment_get5Prime(segment2);
                }
                //If the resulting segment is not null then it must have a block, but be too far to be a feature segment.
                if (segment2 != NULL) {
                    CuAssertTrue(testCase, stHash_search(pinchSegmentsToFeatureSegments, segment2) != NULL);
                    CuAssertTrue(testCase,
                            getDistance(segment2, segment, ignoreUnalignedBases) > maxBaseDistance
                                    || getBlockDistance(segment2, segment, ignoreUnalignedBases) > maxBaseDistance);
                }
                //Now walk 3' until we find a segment that does not have a feature segment
                segment2 = segment;
                while (segment2 != NULL
                        && (stPinchSegment_getBlock(segment2) == NULL
                                || stHash_search(pinchSegmentsToFeatureSegments, segment2) != NULL)) {
                    segment2 = stPinchSegment_get3Prime(segment2);
                }
                //If the resulting segment is not null then it must have a block, but be too far to be a feature segment.
                if (segment2 != NULL) {
                    CuAssertTrue(testCase, stHash_search(pinchSegmentsToFeatureSegments, segment2) != NULL);
                    CuAssertTrue(testCase,
                            getDistance(segment, segment2, ignoreUnalignedBases) > maxBaseDistance
                                    || getBlockDistance(segment, segment2, ignoreUnalignedBases) > maxBaseDistance);
                }
            }

            //Cleanup
            stHash_destruct(pinchSegmentsToFeatureSegments);

            /*
             * Now assess the feature block using passed function.
             */
            if (assessFeatureBlocksFn != NULL) {
                assessFeatureBlocksFn(block, featureBlocks, testCase);
            }

            //Cleanup
            stList_destruct(featureBlocks);
        }

        //Cleanup
        stPinchThreadSet_destruct(randomGraph);
        stHash_destruct(randomStrings);
    }
}

/*
 * Uses the above function to check randomly generated feature blocks are okay.
 */
static void testStFeatureBlock_getContextualFeatureBlocks(CuTest *testCase) {
    makeAndTestRandomFeatureBlocks(testCase, NULL);
}

/*
 * Tests that for a random set of feature blocks we get set of correct feature columns.
 */

static void featureColumnTestFn(stPinchBlock *block, stList *featureBlocks, CuTest *testCase) {
    //Make feature columns
    stList *featureColumns = stFeatureColumn_getFeatureColumns(featureBlocks, block);

    //Check every feature block is covered by a sequence of feature columns
    int64_t j=0;
    for(int64_t i=0; i<stList_length(featureBlocks); i++) {
        stFeatureBlock *featureBlock = stList_get(featureBlocks, i);
        for(int64_t k=0; k<featureBlock->length; k++) {
            stFeatureColumn *featureColumn = stList_get(featureColumns, j++);
            CuAssertIntEquals(testCase, featureColumn->columnIndex,  k);
            CuAssertPtrEquals(testCase, featureColumn->featureBlock, featureBlock);
        }
    }
    CuAssertIntEquals(testCase, stList_length(featureBlocks), j); //We got to the end of the list.

    //Cleanup
    stList_destruct(featureColumns);
}

static void testStFeatureColumn_getFeatureColumns(CuTest *testCase) {
    makeAndTestRandomFeatureBlocks(testCase, featureColumnTestFn);
}

/*
 * Tests that for random sequence graphs the counts of substitutions is accurate.
 */

static double constantDistanceWeightFn(int64_t i, int64_t) {
    //Let's do the simple case.
    return 1;
}

stMatrix *getSubstitutionMatrixSimply(stList *featureColumns) {

}

static void substitutionMatrixTestFn(stPinchBlock *block, stList *featureBlocks, CuTest *testCase) {
    //Make feature columns
    stList *featureColumns = stFeatureColumn_getFeatureColumns(featureBlocks, block);

    //Make substitution matrix
    stMatrix *matrix = stPinchPhylogeny_getMatrixFromSubstitutions(featureColumns, block, constantDistanceWeightFn, 0);

    //Build substitution matrix independently from feature columns
    stMatrix *matrix2 = getSubstitutionMatrixSimply(featureColumns);

    //Now check matrices agree
    CuAssertTrue(testCase, stMatrix_equal(matrix, matrix2));

    //Cleanup
    stMatrix_destruct(matrix);
    stList_destruct(featureColumns);
}

static void testStPinchPhylogeny_getMatrixFromSubstitutions(CuTest *testCase) {
    makeAndTestRandomFeatureBlocks(testCase, substitutionMatrixTestFn);
}
/*
 * Tests that for random sequence graphs the counts of breakpoints is accurate.
 */

stMatrix *getBreakpointMatrixSimply(stList *featureColumns) {

}

static void breakpointMatrixTestFn(stPinchBlock *block, stList *featureBlocks, CuTest *testCase) {
    //Make feature columns
    stList *featureColumns = stFeatureColumn_getFeatureColumns(featureBlocks, block);

    //Make substitution matrix
    stMatrix *matrix = stPinchPhylogeny_getMatrixFromSubstitutions(featureColumns, block, constantDistanceWeightFn, 0);

    //Build substitution matrix independently from feature columns
    stMatrix *matrix2 = getBreakpointMatrixSimply(featureColumns);

    //Now check matrices agree
    CuAssertTrue(testCase, stMatrix_equal(matrix, matrix2));

    //Cleanup
    stMatrix_destruct(matrix);
    stList_destruct(featureColumns);
}

static void testStPinchPhylogeny_getMatrixFromBreakPoints(CuTest *testCase) {
    makeAndTestRandomFeatureBlocks(testCase, breakpointMatrixTestFn);
}

/*
 * Tests that the symmetric distance matrix is calculated as expected.
 */
static void testStPinchPhylogeny_getSymmetricDistanceMatrix(CuTest *testCase) {

}

void testSimpleRemovePoorlySupportedPartitions(CuTest *testCase) {
    int64_t i;
    stTree *tree = stTree_parseNewickString("(((((0,1),(2,3)),4),5),(6,(7,8)));");
    stTree *result;
    stPhylogeny_addStPhylogenyInfo(tree);

    // Running on a tree with no support at all should create a star tree.
    result = removePoorlySupportedPartitions(tree, 1.0);
    CuAssertIntEquals(testCase, 9, stTree_getChildNumber(result));
    for (i = 0; i < stTree_getChildNumber(result); i++) {
        stTree *child = stTree_getChild(result, i);
        CuAssertIntEquals(testCase, 0, stTree_getChildNumber(child));
    }
}

CuSuite* stPinchPhylogenyTestSuite(void) {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, testSimpleRemovePoorlySupportedPartitions);
    SUITE_ADD_TEST(suite, testStFeatureBlock_getContextualFeatureBlocks);
    SUITE_ADD_TEST(suite, testStFeatureColumn_getFeatureColumns);
    SUITE_ADD_TEST(suite, testStPinchPhylogeny_getMatrixFromSubstitutions);
    SUITE_ADD_TEST(suite, testStPinchPhylogeny_getMatrixFromBreakPoints);
    SUITE_ADD_TEST(suite, testStPinchPhylogeny_getSymmetricDistanceMatrix);
    return suite;
}
