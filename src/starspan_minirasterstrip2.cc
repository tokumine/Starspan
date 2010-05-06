//
// StarSpan project
// Carlos A. Rueda
// minirasterstrip2 - similar to minirasters2 but to generate a miniraster strip
//                    from multiple rasters with duplicate pixel handling
// $Id: starspan_minirasterstrip2.cc,v 1.3 2008-05-08 00:42:20 crueda Exp $
//

#include "starspan.h"
#include "traverser.h"

#include <stdlib.h>
#include <assert.h>


static Vector* vect;
static vector<const char*>* select_fields;
static int layernum;
                    
// the common observer while all features are traversed:
static Observer* obs;



static void mrs_extractFunction(ExtractionItem* item) {
    
	// strategy:
	// - Set globalOptions.FID = feature->GetFID();
	// - Set list of rasters to only the one given
	// - Create and initialize a traverser
	// - Register the common observer
    // - traverse
	
	// - Set globalOptions.FID = feature->GetFID();
	globalOptions.FID = item->feature->GetFID();
	
	// - Set list of rasters to only the one given
	vector<const char*> raster_filenames;
	raster_filenames.push_back(item->rasterFilename);
	
	// - Create and initialize a traverser
	Traverser tr;
    
	tr.setVector(vect);
	tr.setLayerNum(layernum);

    Raster raster(item->rasterFilename);  
    tr.addRaster(&raster);

    if ( globalOptions.pix_prop >= 0.0 )
		tr.setPixelProportion(globalOptions.pix_prop);
    
    tr.setVectorSelectionParams(globalOptions.vSelParams);
    
    tr.setDesiredFID(globalOptions.FID);
	tr.setVerbose(globalOptions.verbose);
    tr.setSkipInvalidPolygons(globalOptions.skip_invalid_polys);
    
    // - Register the common observer
	tr.addObserver(obs);

    // - traverse
    tr.traverse();
}

//
//
int starspan_minirasterstrip2(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
    const char* basefilename,
    const char* shpfilename
) {
    vect = _vect;
    select_fields = _select_fields;
    layernum = _layernum;

    
    Vector* outVector = 0;
    OGRLayer* outLayer = 0;
    
    // <shp>
    if ( shpfilename ) {
        // prepare outVector and outLayer:
        
        if ( globalOptions.verbose ) {
            cout<< "starspan_minirasterstrip2: starting creation of output vector " <<shpfilename<< " ...\n";
        }
    
        outVector = Vector::create(shpfilename); 
        if ( !outVector ) {
            // errors should have been written
            return 0;
        }                
    
        // create layer in output vector:
        outLayer = starspan_createLayer(
            vect->getLayer(layernum),
            outVector,
            "mini_raster_strip"
        );
        
        if ( outLayer == NULL ) {
            delete outVector;
            outVector = NULL;
            return 0;
        }
        
        // TODO Add definition for new fields (eg. RID)
        // ...
    }
    // </shp>
    
    
    string prefix = basefilename;
    prefix += "_TMP_PRFX_";
    
    // our own list to be updated and used later to create final strip:
    vector<MRBasicInfo> mrbi_list;

    // this observer will update outVector/outLayer if any, but won't
    // create any strip -- we will do it after the scan of features below
    // and with the help ow our own list of MRBasicInfo elements:
    obs = starspan_getMiniRasterStripObserver2(
        basefilename,
        prefix.c_str(),
        outVector,
        outLayer,
        &mrbi_list
    );
    
    
	bool prevResetReading = Traverser::_resetReading;
	Traverser::_resetReading = false;

    int res = starspan_dup_pixel(
        vect,
        raster_filenames,
        mask_filenames,
        select_fields,
        layernum,
        dupPixelModes,
        mrs_extractFunction
    );
    
	Traverser::_resetReading = prevResetReading;
    
    if ( res ) {
        // problems: messages should have been generated.
    }
    else {
        if ( mrbi_list.size() == 0 ) {
            cout<< "\nstarspan_minirasterstrip2: NOTE: No minirasters were generated.\n"; 
        }
        else {
            // OK: create the strip:
            // use the first raster in the list to get #band and_eq band type:
            Raster rastr(raster_filenames[0]);
            int strip_bands;
            rastr.getSize(NULL, NULL, &strip_bands);
            GDALDataType strip_band_type = rastr.getDataset()->GetRasterBand(1)->GetRasterDataType();
            
            starspan_create_strip(
                strip_band_type,
                strip_bands,
                prefix,
                &mrbi_list,
                basefilename
            );
        }
    }
    
    // release dynamic objects:
    if ( outVector ) {
        delete outVector;
    }
    delete obs;
    
    
    // Done:
    return res;
}

