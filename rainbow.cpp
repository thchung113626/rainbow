/*
 * 	rainbow.cpp
 *
 *  Created on 2012.02.08 by Franky Prog(A)42
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "declares.h"
#include "rainbow.h"
#include "stack.h"

#include "xml_parse_lib.h"

#define dprintf printf
// #undef dprintf

#define MaxLine	1024
#define TAG_S_CMAP "clutteralgsettings"
#define TAG_S_BEAMBLOCKING "beamblocking"
#define TAG_S_VOLUME "volume"
#define TAG_S_SCAN "scan"
#define TAG_S_PARGROUP "pargroup"
#define TAG_S_SLICE "slice"
#define TAG_S_SLICEDATA "slicedata"
#define TAG_S_SENSORINFO "sensorinfo"
#define TAG_S_RADARINFO "radarinfo"

#define TAG_E_CMAP "/clutteralgsettings"
#define TAG_E_BEAMBLOCKING "/beamblocking"
#define TAG_E_VOLUME "/volume"
#define TAG_E_SCAN "/scan"
#define TAG_E_PARGROUP "/pargroup"
#define TAG_E_SLICE "/slice"
#define TAG_E_SLICEDATA "/slicedata"
#define TAG_E_SENSORINFO "/sensorinfo"
#define TAG_E_RADARINFO "/radarinfo"

#define TAG_REFLADJ	"refladj"
#define TAG_THRESHOLD "excessthreshold"
#define TAG_CLSPTHRESH "clspreadthresh"

bool processCmap(FILE* fpIn, CMAP* opMap)
{
#ifdef debug
	printf("process Cmap ......\n");
#endif
	int lnNumLine = 0;
	char *lszTag = new char[MaxLine];
	char *lszContents = new char[MaxLine];
	char *lszTagName = new char[MaxLine];

	bool lbOk = true;
	do
	{
		xml_parse( fpIn, lszTag, lszContents, MaxLine, &lnNumLine);
		if(0 == strcmp(lszTag, TAG_REFLADJ))
			opMap->refladj = atof(lszContents);
		else if(0 == strcmp(lszTag, TAG_THRESHOLD))
			opMap->excessthreshold = atof(lszContents);
		else if(0 == strcmp(lszTag, TAG_CLSPTHRESH))
			opMap->clspreadthresh = atof(lszContents);
		else if(0 == strcmp(lszTag, TAG_E_BEAMBLOCKING)) {
			opMap->mrVol = new RAINBOW;
#ifdef debug
			printf("process Volume ...... \n");
#endif
			lbOk = lbOk & processXmlHeader(fpIn, opMap->mrVol->xmlHeader);
		}
	} while(lszTag[0] != '\0' && !strstr(lszTag, TAG_E_CMAP));

	secure_delete_array(lszTag);
	secure_delete_array(lszContents);
	secure_delete_array(lszTagName);

	return lbOk;
}

bool processXmlHeader(FILE* fpIn, VOLUME& orVolume)
{
	int lnNumLine = 0;
	char *lszTag = new char[MaxLine];
	char *lszContents = new char[MaxLine];
	char *lszTagName = new char[MaxLine];
	char *lszAttrName = new char[MaxLine];
	char *lszValue = new char[MaxLine];
	char *lszData = new char[MaxLine];
	SLICE *lrSlice = new SLICE;
	memset(lrSlice, 0, sizeof(SLICE));

	int lnRayInfoIndex = 0;

	struct node lpTagStack;
	stack_init(&lpTagStack);

	xml_parse( fpIn, lszTag, lszContents, MaxLine, &lnNumLine);
	while(lszTag[0] != '\0' && !strstr(lszTag, TAG_E_VOLUME))
	{

#ifdef dprintf
		dprintf("Processing --> \"%s\" \n", lszTag);
#endif

		xml_grab_tag_name( lszTag, lszTagName, MaxLine);

		if(strcmp(lszTagName, TAG_S_VOLUME) == 0)
			stack_push(&lpTagStack, TAG_S_VOLUME);
		else if(strcmp(lszTagName, TAG_S_SCAN) == 0)
			stack_push(&lpTagStack, TAG_S_SCAN);
		else if(strcmp(lszTagName, TAG_S_PARGROUP) == 0)
			stack_push(&lpTagStack, TAG_S_PARGROUP);
		else if(strcmp(lszTagName, TAG_S_SLICE) == 0)
			stack_push(&lpTagStack, TAG_S_SLICE);
		else if(strcmp(lszTagName, TAG_S_SLICEDATA) == 0)
			stack_push(&lpTagStack, TAG_S_SLICEDATA);
		else if(strcmp(lszTagName, TAG_S_SENSORINFO) == 0)
			stack_push(&lpTagStack, TAG_S_SENSORINFO);
		else if(strcmp(lszTagName, TAG_S_RADARINFO) == 0)
			stack_push(&lpTagStack, TAG_S_RADARINFO);

		xml_grab_attrib( lszTag, lszAttrName, lszValue, MaxLine);
		while(lszValue[0] != '\0')
		{
			tag_select_extraction( strcmp(stack_top(&lpTagStack), lszTag) == 0 ? stack_top(&lpTagStack) : lszTagName,
				lszAttrName, lszValue, &orVolume, lrSlice, &lnRayInfoIndex);
			xml_grab_attrib( lszTag, lszAttrName, lszValue, MaxLine);

		}

		// Finalize self-closing tags that represent a single logical record
		// Ensure attribute order does not affect which record gets populated
		if(strcmp(lszTagName, "rayinfo") == 0) {
			lnRayInfoIndex++;
		}

		if(lszContents[0] != '\0')
		{
			tag_select_extraction(stack_top(&lpTagStack), lszTagName, lszContents,
				&orVolume, lrSlice, &lnRayInfoIndex);
		}

		if(strcmp(lszTagName, TAG_E_VOLUME) == 0 || strcmp(lszTagName, TAG_E_SCAN) == 0
			|| strcmp(lszTagName, TAG_E_PARGROUP) == 0 || strcmp(lszTagName, TAG_E_SLICE) == 0
			|| strcmp(lszTagName, TAG_E_SLICEDATA) == 0 || strcmp(lszTagName, TAG_E_SENSORINFO) == 0
			|| strcmp(lszTagName, TAG_S_RADARINFO) == 0)
		{
			if(strcmp(lszTagName, TAG_E_PARGROUP) == 0)
			{
				int lnNumSlice = orVolume.scan.pargroup.numele;

#ifdef dprintf
				dprintf("Expected number of slice: %d \n", lnNumSlice);
#endif

				orVolume.scan.slice = new SLICE[lnNumSlice];
			}
			else if(strcmp(lszTagName, TAG_E_SLICE) == 0)
			{
				int lnIndex = lrSlice->refid;

#ifdef dprintf
				dprintf("Slice# %d \n", lnIndex);
#endif

				memcpy(&orVolume.scan.slice[lnIndex], lrSlice, sizeof(SLICE));
				memset(lrSlice, 0, sizeof(SLICE));
			}
			stack_pop(&lpTagStack);
		}
		xml_parse( fpIn, lszTag, lszContents, MaxLine, &lnNumLine);
	}

#ifdef dprintf
	print_xml_header(&orVolume);
#endif

	stack_clr(&lpTagStack);
	secure_delete_array(lszTag);
	secure_delete_array(lszContents);
	secure_delete_array(lszTagName);
	secure_delete_array(lszAttrName);
	secure_delete_array(lszValue);
	secure_delete_array(lszData);
	secure_delete(lrSlice);
	return true;
}

void print_xml_header(VOLUME *irVol)
{
	int lnNumSlice = 0;

	printf("\n");
	printf("Xml Header \n");
	printf("Volume \n");
	printf("\t version: %s \n", irVol->version);
	printf("\t datetime: %s \n", irVol->datetime);
	printf("\t type: %s \n", irVol->type);

	printf("\t Scan \n");
	printf("\t\t name: %s \n", irVol->scan.name);
	printf("\t\t time: %s \n", irVol->scan.time);
	printf("\t\t date: %s \n", irVol->scan.date);
	printf("\t\t unitid: %s \n", irVol->scan.unitid);
	printf("\t\t advancedchanged: %d \n", irVol->scan.advancedchanged);
	printf("\t\t detailedchanged: %d \n", irVol->scan.detailedchanged);
	printf("\t\t scantime: %d \n", irVol->scan.scantime);

	print_pargroup_struct(&irVol->scan.pargroup);

	lnNumSlice = irVol->scan.pargroup.numele;
	if(irVol->scan.slice != NULL){
		for(int i=0; i<lnNumSlice; i++)
		{
			print_slice_struct(&irVol->scan.slice[i]);
			print_slicedata(&irVol->scan.slice[i].slicedata);
		}
	}
	print_sensor_info(&irVol->sensorinfo);
}

void print_pargroup_struct(PARGROUP *ipPaGp)
{
	printf("\t\t PARGROUP \n");
	printf("\t\t\t refid: %s \n", ipPaGp->refid);
	printf("\t\t\t multitrip: %d \n", ipPaGp->multitrip);
	printf("\t\t\t numtrip: %d \n", ipPaGp->numtrip);
	printf("\t\t\t triprecovery: %d \n", ipPaGp->triprecovery);
	printf("\t\t\t multitripprfmode: %d \n", ipPaGp->multitripprfmode);
	printf("\t\t\t stoprange: %d \n", ipPaGp->stoprange);
	printf("\t\t\t numele: %d \n", ipPaGp->numele);
	printf("\t\t\t scanstrategy: %s \n", ipPaGp->scanstrategy);
	printf("\t\t\t firstele: %d \n", ipPaGp->firstele);
	printf("\t\t\t lastele: %d \n", ipPaGp->lastele);
	printf("\t\t\t posele: %d \n", ipPaGp->posele);
	printf("\t\t\t startazi: %f \n", ipPaGp->startazi);
	printf("\t\t\t stopazi: %f \n", ipPaGp->stopazi);
	printf("\t\t\t sectorscan: %d \n", ipPaGp->sectorscan);
	printf("\t\t\t posazi: %d \n", ipPaGp->posazi);
	printf("\t\t\t startele: %f \n", ipPaGp->startele);
	printf("\t\t\t stopele: %f \n", ipPaGp->stopele);
	printf("\t\t\t datatypez: %d \n", ipPaGp->datatypez);
	printf("\t\t\t datatypeuz: %d \n", ipPaGp->datatypeuz);
	printf("\t\t\t datatypev: %d \n", ipPaGp->datatypev);
	printf("\t\t\t datatypew: %d \n", ipPaGp->datatypew);
	printf("\t\t\t datatypezdr: %d \n", ipPaGp->datatypezdr);
	printf("\t\t\t datatypephidp: %d \n", ipPaGp->datatypephidp);
	printf("\t\t\t datatypekdp: %d \n", ipPaGp->datatypekdp);
	printf("\t\t\t datatyperhohv: %d \n", ipPaGp->datatyperhohv);
	printf("\t\t\t datatypeldr: %d \n", ipPaGp->datatypeldr);
	printf("\t\t\t gdrx_dp_proc_mode: %s \n", ipPaGp->gdrx_dp_proc_mode);
	printf("\t\t\t rangestep: %f \n", ipPaGp->rangestep);
	printf("\t\t\t rangesamp: %d \n", ipPaGp->rangesamp);
	printf("\t\t\t start_range: %d \n", ipPaGp->start_range);
	printf("\t\t\t highprf: %d \n", ipPaGp->highprf);
	printf("\t\t\t stagger: %s \n", ipPaGp->stagger);
	printf("\t\t\t lowprf: %d \n", ipPaGp->lowprf);
	printf("\t\t\t pw_index: %d \n", ipPaGp->pw_index);
	printf("\t\t\t cf_doppler: %d \n", ipPaGp->cf_doppler);
	printf("\t\t\t filtermode: %s \n", ipPaGp->filtermode);
	printf("\t\t\t fftfilter: %d \n", ipPaGp->fftfilter);
	printf("\t\t\t cf_spatial: %d \n", ipPaGp->cf_spatial);
	printf("\t\t\t speckle: %d \n", ipPaGp->speckle);
	printf("\t\t\t gdrx_clutter_flag_filter: %d \n", ipPaGp->gdrx_clutter_flag_filter);
	printf("\t\t\t cf_dectree: %d \n", ipPaGp->cf_dectree);
	printf("\t\t\t cf_gip: %d \n", ipPaGp->cf_gip);
	printf("\t\t\t filterwidth: %d \n", ipPaGp->filterwidth);
	printf("\t\t\t gdrx_cluttermap: %s \n", ipPaGp->gdrx_cluttermap);
	printf("\t\t\t int_filter_mode: %d \n", ipPaGp->int_filter_mode);
	printf("\t\t\t int_filter_hconf: %d \n", ipPaGp->int_filter_hconf);
	printf("\t\t\t int_filter_vconf: %d \n", ipPaGp->int_filter_vconf);
	printf("\t\t\t int_filter_phaseint: %d \n", ipPaGp->int_filter_phaseint);
	printf("\t\t\t antspeed: %d \n", ipPaGp->antspeed);
	printf("\t\t\t anglestep: %d \n", ipPaGp->anglestep);
	printf("\t\t\t timesamp: %d \n", ipPaGp->timesamp);
	printf("\t\t\t fixselect: %s \n", ipPaGp->fixselect);
	printf("\t\t\t csr: %d \n", ipPaGp->csr);
	printf("\t\t\t sqi: %f \n", ipPaGp->sqi);
	printf("\t\t\t zsqi: %f \n", ipPaGp->zsqi);
	printf("\t\t\t log: %d \n", ipPaGp->log);
	printf("\t\t\t extrarclfile: %s \n", ipPaGp->extrarclfile);
	printf("\t\t\t dualprfmode: %s \n", ipPaGp->dualprfmode);
	printf("\t\t\t prfcorrthr: %d \n", ipPaGp->prfcorrthr);
	printf("\t\t\t prfcorron: %d \n", ipPaGp->prfcorron);
	printf("\t\t\t prfcorrmode: %s \n", ipPaGp->prfcorrmode);
	printf("\t\t\t filterdepth: %s \n", ipPaGp->filterdepth);
	printf("\t\t\t fft_weight: %s \n", ipPaGp->fft_weight);
	printf("\t\t\t spatial_modus: %s \n", ipPaGp->spatial_modus);
	printf("\t\t\t spatial_min: %d \n", ipPaGp->spatial_min);
	printf("\t\t\t spatial_max: %d \n", ipPaGp->spatial_max);
	printf("\t\t\t mask_uz: %d \n", ipPaGp->mask_uz);
	printf("\t\t\t mask_cz: %d \n", ipPaGp->mask_cz);
	printf("\t\t\t mask_v: %d \n", ipPaGp->mask_v);
	printf("\t\t\t mask_w: %d \n", ipPaGp->mask_w);
	printf("\t\t\t gdrx_tripnotchwidth: %f \n", ipPaGp->gdrx_tripnotchwidth);
	printf("\t\t\t gdrx_tripccorthres: %f \n", ipPaGp->gdrx_tripccorthres);
	printf("\t\t\t gdrx_tripsnrthres: %f \n", ipPaGp->gdrx_tripsnrthres);
	printf("\t\t\t gdrx_tripsqithres: %f \n", ipPaGp->gdrx_tripsqithres);
	printf("\t\t\t gdrx_tripratiothres: %f \n", ipPaGp->gdrx_tripratiothres);
	printf("\t\t\t gdrx_tripdftwin: %s \n", ipPaGp->gdrx_tripdftwin);
	printf("\t\t\t phase_coding: %d \n", ipPaGp->phase_coding);
	printf("\t\t\t datatypes: %s \n", ipPaGp->datatypes);
	printf("\t\t\t masterdatatypes: %s \n", ipPaGp->masterdatatypes);
}

void print_slice_struct(SLICE *ipSlice)
{
	printf("\t\t SLICE \n");
	printf("\t\t\t refid: %d \n", ipSlice->refid);

	DYN lrDyn;
	lrDyn = ipSlice->dynz;
	printf("\t\t\t dynz: min %f max %f \n", lrDyn.min, lrDyn.max);

	lrDyn = ipSlice->dynv;
	printf("\t\t\t dynv: min %f max %f \n", lrDyn.min, lrDyn.max);

	lrDyn = ipSlice->dynw;
	printf("\t\t\t dynw: min %f max %f \n", lrDyn.min, lrDyn.max);

	lrDyn = ipSlice->dynzdr;
	printf("\t\t\t dynzdr: min %f max %f \n", lrDyn.min, lrDyn.max);

	lrDyn = ipSlice->dynldr;
	printf("\t\t\t dynldr: min %f max %f \n", lrDyn.min, lrDyn.max);

	lrDyn = ipSlice->dynkdp;
	printf("\t\t\t dynkdp: min %f max %f \n", lrDyn.min, lrDyn.max);

	printf("\t\t\t mask_zdr: %d \n", ipSlice->mask_zdr);
	printf("\t\t\t mask_phidp: %d \n", ipSlice->mask_phidp);
	printf("\t\t\t mask_ldr: %d \n", ipSlice->mask_ldr);
	printf("\t\t\t mask_ukdp: %d \n", ipSlice->mask_ukdp);
	printf("\t\t\t mask_rhohv: %d \n", ipSlice->mask_rhohv);
	printf("\t\t\t kdpautoscale: %d \n", ipSlice->kdpautoscale);
	printf("\t\t\t posangle: %f \n", ipSlice->posangle);
	printf("\t\t\t startangle: %f \n", ipSlice->startangle);
	printf("\t\t\t stopangle: %f \n", ipSlice->stopangle);
	printf("\t\t\t numtrip: %d \n", ipSlice->numtrip);
	printf("\t\t\t triprecovery: %d \n", ipSlice->triprecovery);
	printf("\t\t\t stoprange: %d \n", ipSlice->stoprange);
	printf("\t\t\t start_range: %d \n", ipSlice->start_range);
	printf("\t\t\t rangestep: %f \n", ipSlice->rangestep);
	printf("\t\t\t rangesamp: %f \n", ipSlice->rangesamp);
	printf("\t\t\t highprf: %f \n", ipSlice->highprf);
	printf("\t\t\t stagger: %s \n", ipSlice->stagger);
	printf("\t\t\t lowprf: %f \n", ipSlice->lowprf);
	printf("\t\t\t pw_index: %d \n", ipSlice->pw_index);
	printf("\t\t\t cf_doppler: %d \n", ipSlice->cf_doppler);
	printf("\t\t\t filterwidth: %d \n", ipSlice->filterwidth);
	printf("\t\t\t fftfilter: %d \n", ipSlice->fftfilter);
	printf("\t\t\t cf_spatial: %d \n", ipSlice->cf_spatial);
	printf("\t\t\t speckle: %d \n", ipSlice->speckle);
	printf("\t\t\t statisticalfilter: %d \n", ipSlice->statisticalfilter);
	printf("\t\t\t gdrx_cluttermap: %s \n", ipSlice->gdrx_cluttermap);
	printf("\t\t\t gdrx_clutter_flag_filter: %d \n", ipSlice->gdrx_clutter_flag_filter);
	printf("\t\t\t cf_gip: %d \n", ipSlice->cf_gip);
	printf("\t\t\t cf_gip_width: %f \n", ipSlice->cf_gip_width);
	printf("\t\t\t cf_gip_mode_const: %d \n", ipSlice->cf_gip_mode_const);
	printf("\t\t\t cf_gip_max_width: %f \n", ipSlice->cf_gip_max_width);
	printf("\t\t\t int_filter_mode: %d \n", ipSlice->int_filter_mode);
	printf("\t\t\t int_filter_hconf: %d \n", ipSlice->int_filter_hconf);
	printf("\t\t\t int_filter_vconf: %d \n", ipSlice->int_filter_vconf);
	printf("\t\t\t int_filter_phaseint: %d \n", ipSlice->int_filter_phaseint);
	printf("\t\t\t antspeed: %f \n", ipSlice->antspeed);
	printf("\t\t\t fixselect: %s \n", ipSlice->fixselect);
	printf("\t\t\t anglesync: %d \n", ipSlice->anglesync);
	printf("\t\t\t timesamp: %d \n", ipSlice->timesamp);
	printf("\t\t\t anglestep: %d \n", ipSlice->anglestep);
	printf("\t\t\t asyncoffset: %d \n", ipSlice->asyncoffset);
	printf("\t\t\t csr: %d \n", ipSlice->csr);
	printf("\t\t\t sqi: %f \n", ipSlice->sqi);
	printf("\t\t\t zsqi: %f \n", ipSlice->zsqi);
	printf("\t\t\t log: %d \n", ipSlice->log);
	printf("\t\t\t extrarclfile: %s \n", ipSlice->extrarclfile);
	printf("\t\t\t dualprfmode: %s \n", ipSlice->dualprfmode);
	printf("\t\t\t prfcorrthr: %d \n", ipSlice->prfcorrthr);
	printf("\t\t\t prfcorron: %d \n", ipSlice->prfcorron);
	printf("\t\t\t prfcorrmode: %s \n", ipSlice->prfcorrmode);
	printf("\t\t\t filterdepth: %s \n", ipSlice->filterdepth);
	printf("\t\t\t fft_weight: %s \n", ipSlice->fft_weight);
	printf("\t\t\t spatial_modus: %s \n", ipSlice->spatial_modus);
	printf("\t\t\t spatial_min: %d \n", ipSlice->spatial_min);
	printf("\t\t\t spatial_max: %d \n", ipSlice->spatial_max);
	printf("\t\t\t mask_uz: %d \n", ipSlice->mask_uz);
	printf("\t\t\t mask_cz: %d \n", ipSlice->mask_cz);
	printf("\t\t\t mask_v: %d \n", ipSlice->mask_v);
	printf("\t\t\t mask_w: %d \n", ipSlice->mask_w);
	printf("\t\t\t gdrx_tripnotchwidth: %f \n", ipSlice->gdrx_tripnotchwidth);
	printf("\t\t\t gdrx_tripccorthres: %f \n", ipSlice->gdrx_tripccorthres);
	printf("\t\t\t gdrx_tripsnrthres: %f \n", ipSlice->gdrx_tripsnrthres);
	printf("\t\t\t gdrx_tripsqithres: %f \n", ipSlice->gdrx_tripsqithres);
	printf("\t\t\t gdrx_tripratiothres: %f \n", ipSlice->gdrx_tripratiothres);
	printf("\t\t\t gdrx_tripdftwin: %s \n", ipSlice->gdrx_tripdftwin);
	printf("\t\t\t dp_cf_for_hv: %d \n", ipSlice->dp_cf_for_hv);
	printf("\t\t\t noise_power_dbz: %f \n", ipSlice->noise_power_dbz);
	printf("\t\t\t noise_power_dbz_dpv: %f \n", ipSlice->noise_power_dbz_dpv);
	printf("\t\t\t rspradconst: %s \n", ipSlice->rspradconst);
	printf("\t\t\t rspdphradconst: %s \n", ipSlice->rspdphradconst);
	printf("\t\t\t rspdpvradconst: %s \n", ipSlice->rspdpvradconst);
	printf("\t\t\t gdrxmaxpowkw: %f \n", ipSlice->gdrxmaxpowkw);
	printf("\t\t\t rsptxcorr: %d \n", ipSlice->rsptxcorr);
	printf("\t\t\t gdrxafcmode: %d \n", ipSlice->gdrxafcmode);
	printf("\t\t\t gdrxafcfreq: %f \n", ipSlice->gdrxafcfreq);
	printf("\t\t\t gdrxanctxfreq: %f \n", ipSlice->gdrxanctxfreq);
}

void print_slicedata(SLICEDATA *ipSliceData)
{
	printf("\t\t\t SLICEDATA \n");
	printf("\t\t\t\t time: %s \n", ipSliceData->time);
	printf("\t\t\t\t date: %s \n", ipSliceData->date);
	for(int i=0; i<8; i++) {
		if(strlen(ipSliceData->rayinfo[i].refid) > 0)
			print_ray_info(&ipSliceData->rayinfo[i]);
	}
	print_raw_data(&ipSliceData->rawdata);

}

void print_ray_info(RAYINFO *ipRayInfo)
{
	printf("\t\t\t\t rayinfo: refid %s blobid %d rays %d depth %d \n",
		ipRayInfo->refid, ipRayInfo->blobid, ipRayInfo->rays, ipRayInfo->depth);
}

void print_raw_data(RAWDATA *ipRawData)
{
	printf("\t\t\t\t rawdata: blobid %d rays %d type %s bins %d min %f max %f depth %d \n",
		ipRawData->blobid, ipRawData->rays, ipRawData->type, ipRawData->bins,
		ipRawData->min, ipRawData->max, ipRawData->depth);
}

void print_sensor_info(SENSORINFO *ipInfo)
{
	printf("\t SensorInfo \n");
	printf("\t\t type: %s \n", ipInfo->type);
	printf("\t\t id: %s \n", ipInfo->id);
	printf("\t\t name: %s \n", ipInfo->name);
	printf("\t\t lon: %f \n", ipInfo->lon);
	printf("\t\t lat: %f \n", ipInfo->lat);
	printf("\t\t alt: %f \n", ipInfo->alt);
	printf("\t\t wavelen: %f \n", ipInfo->wavelen);
	printf("\t\t beamwidth: %f \n", ipInfo->beamwidth);
	printf("\n");
}

void print_radar(RAINBOW *ipRainbow)
{
	printf("========== Radar Information ========== \n");
	VOLUME lrHead = ipRainbow->xmlHeader;
	printf("Radar date: %s \n", lrHead.scan.date);
	printf("Radar time: %s \n", lrHead.scan.time);
	printf("Radar name: %s \n", lrHead.sensorinfo.name);
	printf("Radar lat: %.2f \n", lrHead.sensorinfo.lat);
	printf("Radar lon: %.2f \n", lrHead.sensorinfo.lon);
	printf("Radar alt: %.2f \n", lrHead.sensorinfo.alt);
	printf("Radar wave len: %f \n", lrHead.sensorinfo.wavelen);
	printf("Radar beamwidth: %f \n", lrHead.sensorinfo.beamwidth);
	printf("\n");

}

void print_blob_data(RAINBOW *ipRainbow)
{
	if(ipRainbow != NULL)
	{
		int lnNumEle = ipRainbow->xmlHeader.scan.pargroup.numele;
		if(ipRainbow->blobData != NULL)
		{
//			for(int i=0; i<lnNumEle; i++)
			for(int i=0; i<lnNumEle; i+=i+1)
			{
				int lnNumRays = ipRainbow->blobData[i].lnRay_tot;
				SLICE lrInfo = ipRainbow->xmlHeader.scan.slice[i];
				printf("========== Sweep Information ========== \n");
				printf("Sweep # %d \n", lrInfo.refid);
				printf("sweep date: %s \n", lrInfo.slicedata.date);
				printf("sweep time: %s \n", lrInfo.slicedata.time);
				printf("Sweep Elevation: %f \n", lrInfo.posangle);
				printf("Sweep range_step %.4f km \n", lrInfo.rangestep);
				RAWDATA lrRaw = lrInfo.slicedata.rawdata;
				printf("Sweep data_type: %s \n", lrRaw.type);
				printf("Sweep data_range: [%.2f, %.2f] \n", lrRaw.min, lrRaw.max);
//				for(int j=0; j<lnNumRays; j++)
				for(int j=0; j<lnNumRays; j+=30)
				{
					int lnNumBins = ipRainbow->blobData[i].lpRay[j].lnBin_tot;
					double ldAzimuth = ipRainbow->blobData[i].lpRay[j].ldAzimuth;
					double ldeAzimuth = ipRainbow->blobData[i].lpRay[j].ldeAzimuth;
					printf("Ray # %3d / %3d Az: %3.2f, %3.2f \n", j, lrRaw.rays, ldAzimuth, ldeAzimuth);
					if(1)
					{
						for(int k=0; k<lnNumBins; k+=10)
						{
							for(int m=k; m<k+10 && m < lnNumBins; m++){
								BINS lrData = ipRainbow->blobData[i].lpRay[j].lpData[m];
								if(lrData == RB_INVALID)
									printf("%4d (---.--) \t", m);
								else
									printf("%4d (%3.2f) \t", m, lrData);
							}
							printf("\n");
						}
					}
					printf("\n");
				}
			}
		}
	}
}

void convert_dd_to_dms(double idDeg, int& onDeg, int& onMin, int& onSec)
{
	double ldCoordinate = idDeg;
	double ldDelta;

	while(ldCoordinate < -180.00)
		ldCoordinate += 360.00;
	while(ldCoordinate > 180.00)
		ldCoordinate -= 360.00;

	ldCoordinate = fabs(ldCoordinate);

	onDeg = (int) ldCoordinate;
	ldDelta = ldCoordinate - onDeg;

	int lnSec = (int) floor(3600.0 * ldDelta);
	onSec = lnSec % 60;
	onMin = (int) floor(lnSec / 60.0);
}

bool write_radar(FILE *fw, RAINBOW *ipRainbow)
{
	if(fw == NULL || ipRainbow == NULL)
		return false;

	int lnVol = 24;
	VOLUME lrHead = ipRainbow->xmlHeader;
	fprintf(fw, "<!-- Radar Information -->\n");
	fprintf(fw, "date:%s\n", lrHead.scan.date);
	fprintf(fw, "time:%s\n", lrHead.scan.time);
	fprintf(fw, "radar_type:uf\n");
	fprintf(fw, "nvolumes:%d\n", lnVol);
	fprintf(fw, "number:0\n");
	fprintf(fw, "name:%s   %s\n", lrHead.sensorinfo.name, lrHead.sensorinfo.name);
	fprintf(fw, "radar_name:%s\n", lrHead.sensorinfo.name);
	fprintf(fw, "project:\n");
	fprintf(fw, "city:\n");
	fprintf(fw, "state:\n");

	int lnLatd, lnLatm, lnLats;
	int lnLond, lnLonm, lnLons;
	convert_dd_to_dms(lrHead.sensorinfo.lat, lnLatd, lnLatm, lnLats);
	convert_dd_to_dms(lrHead.sensorinfo.lon, lnLond, lnLonm, lnLons);

	fprintf(fw, "latd:%d\n", lnLatd);
	fprintf(fw, "latm:%d\n", lnLatm);
	fprintf(fw, "lats:%d\n", lnLats);
	fprintf(fw, "lond:%d\n", lnLond);
	fprintf(fw, "lonm:%d\n", lnLonm);
	fprintf(fw, "lons:%d\n", lnLons);
	fprintf(fw, "height:%d\n", (int)ceil(lrHead.sensorinfo.alt));
	fprintf(fw, "spulse:0\n");
	fprintf(fw, "lpulse:0\n");
	fprintf(fw, "vcp:0\n");
	fprintf(fw, "<!-- Radar ends -->\n");

	// for(int i=0; i<lnVol; i++) {
		int lnNumSweep = lrHead.scan.pargroup.numele;
		double ldConstr = 82.239998;
		fprintf(fw, "<!-- Volume Information -->\n");
		fprintf(fw, "type_str:\n");
		fprintf(fw, "nsweep:%d\n", lnNumSweep);
		fprintf(fw, "calibr_constr:%f\n", ldConstr);
		fprintf(fw, "<!-- Volume ends -->\n");

		if(ipRainbow->blobData == NULL)
			return false;

		for(int j=0; j<lnNumSweep; j++) {
			int lnRay_tot = ipRainbow->blobData[j].lnRay_tot;
			double ldBW = lrHead.sensorinfo.beamwidth;

			fprintf(fw, "<!-- Sweep Information -->\n");
			fprintf(fw, "sweep_num:%d\n", j+1);
			fprintf(fw, "elev:%f\n", lrHead.scan.slice[j].posangle);
			fprintf(fw, "beam_width:%f\n", ldBW);
			fprintf(fw, "vert_half_bw:%f\n", ldBW/2);
			fprintf(fw, "horz_half_bw:%f\n", ldBW/2);
			fprintf(fw, "nrays:%d\n", lnRay_tot);
			fprintf(fw, "<!-- Sweeps ends -->\n");

			if(ipRainbow->blobData[j].lpRay == NULL)
				return false;

			for(int k=0; k<lnRay_tot; k++) {
				int lnNumBins = ipRainbow->blobData[j].lpRay[k].lnBin_tot;

				fprintf(fw, "<!-- Ray Information -->\n");
				fprintf(fw, "ray_num:%d\n", k+1);
				fprintf(fw, "date:%s\n", lrHead.scan.slice[j].slicedata.date);
				fprintf(fw, "time:%s\n", lrHead.scan.slice[j].slicedata.time);
				fprintf(fw, "unam:%f\n", lrHead.scan.pargroup.stoprange * 1.0);
				fprintf(fw, "azimuth:%f\n", ipRainbow->blobData[j].lpRay[k].ldAzimuth);
				fprintf(fw, "elev:%f\n", lrHead.scan.slice[j].posangle);
				fprintf(fw, "elev_num:%d\n", 1);
				fprintf(fw, "range_bin1:%d\n", lrHead.scan.slice[j].start_range);
				fprintf(fw, "gate_size:%d\n", (int)(lrHead.scan.slice[j].rangestep*1000));
				fprintf(fw, "vel_res:%f\n", 0.0);
				fprintf(fw, "sweep_rate:%f\n", 4.0);
				fprintf(fw, "prf:%f\n", 4.0);
				fprintf(fw, "azim_rate:%f\n", 24.0);
				fprintf(fw, "fix_angle:%f\n", 2.0);
				fprintf(fw, "pulse_count:%f\n", 2.0);
				fprintf(fw, "pulse_width:%f\n", 1.000692);
				fprintf(fw, "beam_width:%f\n", 0.890625);
				fprintf(fw, "frequency:%f\n", 15.625);
				fprintf(fw, "wavelength:%f\n", lrHead.sensorinfo.wavelen);
				fprintf(fw, "nyq_vel:%f\n", 15.89);
				fprintf(fw, "latitude:%f\n", lrHead.sensorinfo.lat);
				fprintf(fw, "longitude:%f\n", lrHead.sensorinfo.lon);
				fprintf(fw, "nbins:%d\n", lnNumBins);
				fprintf(fw, "<!-- Rays ends -->\n");

				if(ipRainbow->blobData[j].lpRay[k].lpData == NULL)
					return false;

				fprintf(fw, "<!-- Bins Information -->\n");
				for(int m=0; m<lnNumBins; m++) {
					fprintf(fw, "%d:%f\n", m, ipRainbow->blobData[j].lpRay[k].lpData[m]);
				}
				fprintf(fw, "<!-- Bins ends -->\n");
			}
		}
	// }
	return true;
}

void tag_select_extraction(char* iszTop, char* iszTag, char* iszData,
	VOLUME* orVol, SLICE *orSlice, int *onRayInfoIndex)
{
	if(iszTop[0]=='\0' || iszTag[0]=='\0' || iszData[0]=='\0')
		return;

	if(orVol == NULL || orSlice == NULL )
		return;

	if(strcmp(iszTop, TAG_S_VOLUME) == 0)
		extract_volume(iszTag, iszData, orVol);
	else if(strcmp(iszTop, TAG_S_SCAN) == 0)
		extract_scan(iszTag, iszData, &orVol->scan);
	else if(strcmp(iszTop, TAG_S_PARGROUP) == 0)
		extract_pargroup(iszTag, iszData, &orVol->scan.pargroup);
	else if(strcmp(iszTop, TAG_S_SLICE) == 0)
		extract_slice(iszTag, iszData, orSlice);
	else if(strcmp(iszTop, TAG_S_SLICEDATA) == 0)
		extract_slice_data(iszTag, iszData, &orSlice->slicedata);
	else if(strcmp(iszTop, TAG_S_SENSORINFO) == 0)
		extract_sensorinfo(iszTag, iszData, &orVol->sensorinfo);
	else if(strcmp(iszTop, TAG_S_RADARINFO) == 0)
		extract_sensorinfo(iszTag, iszData, &orVol->sensorinfo);
	else
		extract_other(iszTop, iszTag, iszData, orSlice, onRayInfoIndex);
}

void extract_volume(char *iszTag, char* iszData, VOLUME *orVol)
{

#ifdef dprintf
	dprintf("--> volume \"%s\" \"%s\" \n", iszTag, iszData);
#endif

	if(strcmp(iszTag, "version") == 0)
		strcpy(orVol->version, iszData);
	else if(strcmp(iszTag, "datetime") == 0)
		strcpy(orVol->datetime, iszData);
	else if(strcmp(iszTag, "type") == 0)
		strcpy(orVol->type, iszData);
	else if(strcmp(iszTag, "owner") == 0)
		strcpy(orVol->owner, iszData);
	else if(strcmp(iszTag, "history") == 0)
		strcpy(orVol->history, iszData);
}

void extract_scan(char *iszTag, char* iszData, SCAN *orScan)
{

#ifdef dprintf
	dprintf("--> scan \"%s\" \"%s\" \n", iszTag, iszData);
#endif

	if(strcmp(iszTag, "name") == 0)
		strcpy(orScan->name, iszData);
	else if(strcmp(iszTag, "time") == 0)
		strcpy(orScan->time, iszData);
	else if(strcmp(iszTag, "date") == 0)
		strcpy(orScan->date, iszData);
	else if(strcmp(iszTag, "unitid") == 0)
		strcpy(orScan->unitid, iszData);
	else if(strcmp(iszTag, "advancedchanged") == 0)
		orScan->advancedchanged = atoi(iszData);
	else if(strcmp(iszTag, "detailedchanged") == 0)
		orScan->detailedchanged = atoi(iszData);
	else if(strcmp(iszTag, "scantime") == 0)
		orScan->scantime = atoi(iszData);
}

void extract_pargroup(char *iszTag, char* iszData, PARGROUP *orPargroup)
{

#ifdef dprintf
	printf("--> pargroup \"%s\" \"%s\" \n", iszTag, iszData);
#endif

	if(strcmp(iszTag, "refid") == 0)
		strcpy(orPargroup->refid, iszData);

	else if(strcmp(iszTag, "multitrip") == 0)
		orPargroup->multitrip = return_boolean(iszData);

	else if(strcmp(iszTag, "numtrip") == 0)
		orPargroup->numtrip = atoi(iszData);

	else if(strcmp(iszTag, "triprecovery") == 0)
		orPargroup->triprecovery =return_boolean(iszData);

	else if(strcmp(iszTag, "multitripprfmode") == 0)
		orPargroup->multitripprfmode =return_boolean(iszData);

	else if(strcmp(iszTag, "stoprange") == 0)
		orPargroup->stoprange =atoi(iszData);

	else if(strcmp(iszTag, "numele") == 0)
		orPargroup->numele =atoi(iszData);

	else if(strcmp(iszTag, "scanstrategy") == 0)
		strcpy(orPargroup->scanstrategy, iszData);

	else if(strcmp(iszTag, "firstele") == 0)
		orPargroup->firstele =atoi(iszData);

	else if(strcmp(iszTag, "lastele") == 0)
		orPargroup->lastele =atoi(iszData);

	else if(strcmp(iszTag, "posele") == 0)
		orPargroup->posele =atoi(iszData);

	else if(strcmp(iszTag, "startazi") == 0)
		orPargroup->startazi =atof(iszData);

	else if(strcmp(iszTag, "stopazi") == 0)
		orPargroup->stopazi =atof(iszData);

	else if(strcmp(iszTag, "sectorscan") == 0)
		orPargroup->sectorscan =atoi(iszData);

	else if(strcmp(iszTag, "posazi") == 0)
		orPargroup->posazi =atoi(iszData);

	else if(strcmp(iszTag, "startele") == 0)
		orPargroup->startele =atof(iszData);

	else if(strcmp(iszTag, "stopele") == 0)
		orPargroup->stopele =atof(iszData);

	else if(strcmp(iszTag, "datatypez") == 0)
		orPargroup->datatypez =atoi(iszData);

	else if(strcmp(iszTag, "datatypeuz") == 0)
		orPargroup->datatypeuz =atoi(iszData);

	else if(strcmp(iszTag, "datatypev") == 0)
		orPargroup->datatypev =atoi(iszData);

	else if(strcmp(iszTag, "datatypew") == 0)
		orPargroup->datatypew =atoi(iszData);

	else if(strcmp(iszTag, "datatypezdr") == 0)
		orPargroup->datatypezdr =atoi(iszData);

	else if(strcmp(iszTag, "datatypephidp") == 0)
		orPargroup->datatypephidp =atoi(iszData);

	else if(strcmp(iszTag, "datatypekdp") == 0)
		orPargroup->datatypekdp =atoi(iszData);

	else if(strcmp(iszTag, "datatyperhohv") == 0)
		orPargroup->datatyperhohv =atoi(iszData);

	else if(strcmp(iszTag, "datatypeldr") == 0)
		orPargroup->datatypeldr =atoi(iszData);

	else if(strcmp(iszTag, "gdrx_dp_proc_mode") == 0)
		strcpy(orPargroup->gdrx_dp_proc_mode, iszData);

	else if(strcmp(iszTag, "rangestep") == 0)
		orPargroup->rangestep =atof(iszData);

	else if(strcmp(iszTag, "rangesamp") == 0)
		orPargroup->rangesamp =atoi(iszData);

	else if(strcmp(iszTag, "start_range") == 0)
		orPargroup->start_range =atoi(iszData);

	else if(strcmp(iszTag, "highprf") == 0)
		orPargroup->highprf =atoi(iszData);

	else if(strcmp(iszTag, "stagger") == 0)
		strcpy(orPargroup->stagger, iszData);

	else if(strcmp(iszTag, "lowprf") == 0)
		orPargroup->lowprf =atoi(iszData);

	else if(strcmp(iszTag, "pw_index") == 0)
		orPargroup->pw_index =atoi(iszData);

	else if(strcmp(iszTag, "cf_doppler") == 0)
		orPargroup->cf_doppler =return_boolean(iszData);

	else if(strcmp(iszTag, "filtermode") == 0)
		strcpy(orPargroup->filtermode, iszData);

	else if(strcmp(iszTag, "fftfilter") == 0)
		orPargroup->fftfilter =return_boolean(iszData);

	else if(strcmp(iszTag, "cf_spatial") == 0)
		orPargroup->cf_spatial =return_boolean(iszData);

	else if(strcmp(iszTag, "speckle") == 0)
		orPargroup->speckle =return_boolean(iszData);

	else if(strcmp(iszTag, "gdrx_clutter_flag_filter") == 0)
		orPargroup->gdrx_clutter_flag_filter =return_boolean(iszData);

	else if(strcmp(iszTag, "cf_dectree") == 0)
		orPargroup->cf_dectree =return_boolean(iszData);

	else if(strcmp(iszTag, "cf_gip") == 0)
		orPargroup->cf_gip =return_boolean(iszData);

	else if(strcmp(iszTag, "filterwidth") == 0)
		orPargroup->filterwidth =atoi(iszData);

	else if(strcmp(iszTag, "gdrx_cluttermap") == 0)
		strcpy(orPargroup->gdrx_cluttermap, iszData);

	else if(strcmp(iszTag, "int_filter_mode") == 0)
		orPargroup->int_filter_mode =atoi(iszData);

	else if(strcmp(iszTag, "int_filter_hconf") == 0)
		orPargroup->int_filter_hconf =atoi(iszData);

	else if(strcmp(iszTag, "int_filter_vconf") == 0)
		orPargroup->int_filter_vconf =atoi(iszData);

	else if(strcmp(iszTag, "int_filter_phaseint") == 0)
		orPargroup->int_filter_phaseint =return_boolean(iszData);

	else if(strcmp(iszTag, "antspeed") == 0)
		orPargroup->antspeed =atoi(iszData);

	else if(strcmp(iszTag, "anglestep") == 0)
		orPargroup->anglestep =atoi(iszData);

	else if(strcmp(iszTag, "timesamp") == 0)
		orPargroup->timesamp =atoi(iszData);

	else if(strcmp(iszTag, "fixselect") == 0)
		strcpy(orPargroup->fixselect, iszData);

	else if(strcmp(iszTag, "csr") == 0)
		orPargroup->csr =atoi(iszData);

	else if(strcmp(iszTag, "sqi") == 0)
		orPargroup->sqi =atof(iszData);

	else if(strcmp(iszTag, "zsqi") == 0)
		orPargroup->zsqi =atof(iszData);

	else if(strcmp(iszTag, "log") == 0)
		orPargroup->log =atoi(iszData);

	else if(strcmp(iszTag, "extrarclfile") == 0)
		strcpy(orPargroup->extrarclfile, iszData);

	else if(strcmp(iszTag, "dualprfmode") == 0)
		strcpy(orPargroup->dualprfmode, iszData);

	else if(strcmp(iszTag, "prfcorrthr") == 0)
		orPargroup->prfcorrthr =atoi(iszData);

	else if(strcmp(iszTag, "prfcorron") == 0)
		orPargroup->prfcorron =return_boolean(iszData);

	else if(strcmp(iszTag, "prfcorrmode") == 0)
		strcpy(orPargroup->prfcorrmode, iszData);

	else if(strcmp(iszTag, "filterdepth") == 0)
		strcpy(orPargroup->filterdepth, iszData);

	else if(strcmp(iszTag, "fft_weight") == 0)
		strcpy(orPargroup->fft_weight, iszData);

	else if(strcmp(iszTag, "spatial_modus") == 0)
		strcpy(orPargroup->spatial_modus, iszData);

	else if(strcmp(iszTag, "spatial_min") == 0)
		orPargroup->spatial_min =atoi(iszData);

	else if(strcmp(iszTag, "spatial_max") == 0)
		orPargroup->spatial_max =atoi(iszData);

	else if(strcmp(iszTag, "mask_uz") == 0)
		orPargroup->mask_uz =atoi(iszData);

	else if(strcmp(iszTag, "mask_cz") == 0)
		orPargroup->mask_cz =atoi(iszData);

	else if(strcmp(iszTag, "mask_v") == 0)
		orPargroup->mask_v =atoi(iszData);

	else if(strcmp(iszTag, "mask_w") == 0)
		orPargroup->mask_w =atoi(iszData);

	else if(strcmp(iszTag, "gdrx_tripnotchwidth") == 0)
		orPargroup->gdrx_tripnotchwidth =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripccorthres") == 0)
		orPargroup->gdrx_tripccorthres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripsnrthres") == 0)
		orPargroup->gdrx_tripsnrthres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripsqithres") == 0)
		orPargroup->gdrx_tripsqithres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripratiothres") == 0)
		orPargroup->gdrx_tripratiothres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripdftwin") == 0)
		strcpy(orPargroup->gdrx_tripdftwin, iszData);

	else if(strcmp(iszTag, "phase_coding") == 0)
		orPargroup->phase_coding =atoi(iszData);

	else if(strcmp(iszTag, "datatypes") == 0)
		strcpy(orPargroup->datatypes, iszData);

	else if(strcmp(iszTag, "masterdatatypes") == 0)
		strcpy(orPargroup->masterdatatypes, iszData);
}

void extract_sensorinfo(char *iszTag, char* iszData, SENSORINFO* orSensorinfo)
{

#ifdef dprintf
	dprintf("--> sensorinfo \"%s\" \"%s\" \n", iszTag, iszData);
#endif

	if(strcmp(iszTag, "type") == 0)
		strcpy(orSensorinfo->type, iszData);
	else if(strcmp(iszTag, "id") == 0)
		strcpy(orSensorinfo->id, iszData);
	else if(strcmp(iszTag, "name") == 0)
		strcpy(orSensorinfo->name, iszData);
	else if(strcmp(iszTag, "lon") == 0)
		orSensorinfo->lon = atof(iszData);
	else if(strcmp(iszTag, "lat") == 0)
		orSensorinfo->lat = atof(iszData);
	else if(strcmp(iszTag, "alt") == 0)
		orSensorinfo->alt = atof(iszData);
	else if(strcmp(iszTag, "wavelen") == 0)
		orSensorinfo->wavelen = atof(iszData);
	else if(strcmp(iszTag, "beamwidth") == 0)
		orSensorinfo->beamwidth = atof(iszData);

}

void extract_slice(char *iszTag, char* iszData, SLICE *orSlice)
{

#ifdef dprintf
	printf("--> slice \"%s\" \"%s\" \n", iszTag, iszData);
#endif

	if(strcmp(iszTag, "refid") == 0)
		orSlice->refid = atoi(iszData);

	else if(strcmp(iszTag, "mask_zdr") == 0)
		orSlice->mask_zdr =atoi(iszData);

	else if(strcmp(iszTag, "mask_phidp") == 0)
		orSlice->mask_phidp =atoi(iszData);

	else if(strcmp(iszTag, "mask_ldr") == 0)
		orSlice->mask_ldr =atoi(iszData);

	else if(strcmp(iszTag, "mask_ukdp") == 0)
		orSlice->mask_ukdp =atoi(iszData);

	else if(strcmp(iszTag, "mask_rhohv") == 0)
		orSlice->mask_rhohv =atoi(iszData);

	else if(strcmp(iszTag, "kdpautoscale") == 0)
		orSlice->kdpautoscale = return_boolean(iszData);

	else if(strcmp(iszTag, "posangle") == 0)
		orSlice->posangle =atof(iszData);

	else if(strcmp(iszTag, "startangle") == 0)
		orSlice->startangle =atof(iszData);

	else if(strcmp(iszTag, "stopangle") == 0)
		orSlice->stopangle =atof(iszData);

	else if(strcmp(iszTag, "numtrip") == 0)
		orSlice->numtrip = atoi(iszData);

	else if(strcmp(iszTag, "triprecovery") == 0)
		orSlice->triprecovery =return_boolean(iszData);

	else if(strcmp(iszTag, "stoprange") == 0)
		orSlice->stoprange =atoi(iszData);

	else if(strcmp(iszTag, "start_range") == 0)
		orSlice->start_range =atoi(iszData);

	else if(strcmp(iszTag, "rangestep") == 0)
		orSlice->rangestep =atof(iszData);

	else if(strcmp(iszTag, "rangesamp") == 0)
		orSlice->rangesamp =atoi(iszData);

	else if(strcmp(iszTag, "highprf") == 0)
		orSlice->highprf =atoi(iszData);

	else if(strcmp(iszTag, "stagger") == 0)
		strcpy(orSlice->stagger, iszData);

	else if(strcmp(iszTag, "lowprf") == 0)
		orSlice->lowprf =atoi(iszData);

	else if(strcmp(iszTag, "pw_index") == 0)
		orSlice->pw_index =atoi(iszData);

	else if(strcmp(iszTag, "cf_doppler") == 0)
		orSlice->cf_doppler =return_boolean(iszData);

	else if(strcmp(iszTag, "filterwidth") == 0)
		orSlice->filterwidth =atoi(iszData);

	else if(strcmp(iszTag, "fftfilter") == 0)
		orSlice->fftfilter =return_boolean(iszData);

	else if(strcmp(iszTag, "cf_spatial") == 0)
		orSlice->cf_spatial =return_boolean(iszData);

	else if(strcmp(iszTag, "speckle") == 0)
		orSlice->speckle =return_boolean(iszData);

	else if(strcmp(iszTag, "statisticalfilter") == 0)
		orSlice->statisticalfilter =return_boolean(iszData);

	else if(strcmp(iszTag, "gdrx_cluttermap") == 0)
		strcpy(orSlice->gdrx_cluttermap, iszData);

	else if(strcmp(iszTag, "gdrx_clutter_flag_filter") == 0)
		orSlice->gdrx_clutter_flag_filter =return_boolean(iszData);

	else if(strcmp(iszTag, "cf_gip") == 0)
		orSlice->cf_gip =return_boolean(iszData);

	else if(strcmp(iszTag, "cf_gip_width") == 0)
		orSlice->cf_gip_width =atof(iszData);

	else if(strcmp(iszTag, "cf_gip_mode_const") == 0)
		orSlice->cf_gip_mode_const =return_boolean(iszData);

	else if(strcmp(iszTag, "cf_gip_max_width") == 0)
		orSlice->cf_gip_max_width =atof(iszData);

	else if(strcmp(iszTag, "int_filter_mode") == 0)
		orSlice->int_filter_mode =atoi(iszData);

	else if(strcmp(iszTag, "int_filter_hconf") == 0)
		orSlice->int_filter_hconf =atoi(iszData);

	else if(strcmp(iszTag, "int_filter_vconf") == 0)
		orSlice->int_filter_vconf =atoi(iszData);

	else if(strcmp(iszTag, "int_filter_phaseint") == 0)
		orSlice->int_filter_phaseint =return_boolean(iszData);

	else if(strcmp(iszTag, "antspeed") == 0)
		orSlice->antspeed =atoi(iszData);

	else if(strcmp(iszTag, "fixselect") == 0)
		strcpy(orSlice->fixselect, iszData);

	else if(strcmp(iszTag, "anglesync") == 0)
		orSlice->anglesync =return_boolean(iszData);

	else if(strcmp(iszTag, "timesamp") == 0)
		orSlice->timesamp =atoi(iszData);

	else if(strcmp(iszTag, "anglestep") == 0)
		orSlice->anglestep =atoi(iszData);

	else if(strcmp(iszTag, "asyncoffset") == 0)
		orSlice->asyncoffset =atoi(iszData);

	else if(strcmp(iszTag, "csr") == 0)
		orSlice->csr =atoi(iszData);

	else if(strcmp(iszTag, "sqi") == 0)
		orSlice->sqi =atof(iszData);

	else if(strcmp(iszTag, "zsqi") == 0)
		orSlice->zsqi =atof(iszData);

	else if(strcmp(iszTag, "log") == 0)
		orSlice->log =atoi(iszData);

	else if(strcmp(iszTag, "extrarclfile") == 0)
		strcpy(orSlice->extrarclfile, iszData);

	else if(strcmp(iszTag, "dualprfmode") == 0)
		strcpy(orSlice->dualprfmode, iszData);

	else if(strcmp(iszTag, "prfcorrthr") == 0)
		orSlice->prfcorrthr =atoi(iszData);

	else if(strcmp(iszTag, "prfcorron") == 0)
		orSlice->prfcorron =return_boolean(iszData);

	else if(strcmp(iszTag, "prfcorrmode") == 0)
		strcpy(orSlice->prfcorrmode, iszData);

	else if(strcmp(iszTag, "filterdepth") == 0)
		strcpy(orSlice->filterdepth, iszData);

	else if(strcmp(iszTag, "fft_weight") == 0)
		strcpy(orSlice->fft_weight, iszData);

	else if(strcmp(iszTag, "spatial_modus") == 0)
		strcpy(orSlice->spatial_modus, iszData);

	else if(strcmp(iszTag, "spatial_min") == 0)
		orSlice->spatial_min =atoi(iszData);

	else if(strcmp(iszTag, "spatial_max") == 0)
		orSlice->spatial_max =atoi(iszData);

	else if(strcmp(iszTag, "mask_uz") == 0)
		orSlice->mask_uz =atoi(iszData);

	else if(strcmp(iszTag, "mask_cz") == 0)
		orSlice->mask_cz =atoi(iszData);

	else if(strcmp(iszTag, "mask_v") == 0)
		orSlice->mask_v =atoi(iszData);

	else if(strcmp(iszTag, "mask_w") == 0)
		orSlice->mask_w =atoi(iszData);

	else if(strcmp(iszTag, "gdrx_tripnotchwidth") == 0)
		orSlice->gdrx_tripnotchwidth =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripccorthres") == 0)
		orSlice->gdrx_tripccorthres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripsnrthres") == 0)
		orSlice->gdrx_tripsnrthres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripsqithres") == 0)
		orSlice->gdrx_tripsqithres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripratiothres") == 0)
		orSlice->gdrx_tripratiothres =atof(iszData);

	else if(strcmp(iszTag, "gdrx_tripdftwin") == 0)
		strcpy(orSlice->gdrx_tripdftwin, iszData);

	else if(strcmp(iszTag, "dp_cf_for_hv") == 0)
		orSlice->dp_cf_for_hv =return_boolean(iszData);

	else if(strcmp(iszTag, "noise_power_dbz") == 0)
		orSlice->noise_power_dbz =atof(iszData);

	else if(strcmp(iszTag, "noise_power_dbz_dpv") == 0)
		orSlice->noise_power_dbz_dpv =atof(iszData);

	else if(strcmp(iszTag, "rspradconst") == 0)
		strcpy(orSlice->rspradconst, iszData);

	else if(strcmp(iszTag, "rspdphradconst") == 0)
		strcpy(orSlice->rspdphradconst, iszData);

	else if(strcmp(iszTag, "rspdpvradconst") == 0)
		strcpy(orSlice->rspdpvradconst, iszData);

	else if(strcmp(iszTag, "gdrxmaxpowkw") == 0)
		orSlice->gdrxmaxpowkw =atof(iszData);

	else if(strcmp(iszTag, "rsptxcorr") == 0)
		orSlice->rsptxcorr =atoi(iszData);

	else if(strcmp(iszTag, "gdrxafcmode") == 0)
			orSlice->gdrxafcmode =atoi(iszData);

	else if(strcmp(iszTag, "gdrxafcfreq") == 0)
		orSlice->gdrxafcfreq =atof(iszData);

	else if(strcmp(iszTag, "gdrxanctxfreq") == 0)
		orSlice->gdrxanctxfreq =atof(iszData);

}

void extract_slice_data(char *iszTag, char* iszData, SLICEDATA *orSliceData)
{

#ifdef dprintf
	dprintf("--> slice-data \"%s\" \"%s\" \n", iszTag, iszData);
#endif

	if(strcmp(iszTag, "time") == 0)
		strcpy(orSliceData->time, iszData);
	else if(strcmp(iszTag, "date") == 0)
		strcpy(orSliceData->date, iszData);
}

void extract_other(char *iszTop, char *iszTag, char *iszData, SLICE *orSlice, int *onRayInfoIndex)
{

#ifdef dprintf
	printf("--> other \"%s\" \"%s\" \"%s\" \n", iszTop, iszTag, iszData);
#endif

	if(strcmp(iszTop, "rayinfo") == 0)
		extract_rayinfo(iszTag, iszData, &orSlice->slicedata.rayinfo[(*onRayInfoIndex)], onRayInfoIndex);
	else if(strcmp(iszTop, "rawdata") == 0) {
		extract_rawdata(iszTag, iszData, &orSlice->slicedata.rawdata);
		(*onRayInfoIndex) = 0;
	} else if(strlen(iszTop) > 3 && iszTop[0]=='d' && iszTop[1]=='y' && iszTop[2]=='n')
		extract_dyn_type(iszTop, iszTag, iszData, orSlice);
}

void extract_rayinfo(char *iszTag, char *iszData, RAYINFO *orRayinfo, int *onRayInfoIndex)
{
	if(strcmp(iszTag, "refid")==0)
		strcpy(orRayinfo->refid, iszData);
	else if(strcmp(iszTag, "blobid")==0)
		orRayinfo->blobid = atoi(iszData);
	else if(strcmp(iszTag, "rays")==0)
		orRayinfo->rays = atoi(iszData);
	else if(strcmp(iszTag, "depth")==0)
		orRayinfo->depth = atoi(iszData);
}

void extract_rawdata(char *iszTag, char *iszData, RAWDATA *orRawdata)
{
	if(strcmp(iszTag, "blobid")==0)
		orRawdata->blobid = atoi(iszData);
	else if(strcmp(iszTag, "rays")==0)
		orRawdata->rays = atoi(iszData);
	else if(strcmp(iszTag, "type")==0)
		strcpy(orRawdata->type, iszData);
	else if(strcmp(iszTag, "bins")==0)
		orRawdata->bins = atoi(iszData);
	else if(strcmp(iszTag, "min")==0)
		orRawdata->min = atof(iszData);
	else if(strcmp(iszTag, "max")==0)
		orRawdata->max = atof(iszData);
	else if(strcmp(iszTag, "depth")==0)
		orRawdata->depth = atoi(iszData);
}

void extract_dyn_type(char *iszTop, char *iszTag, char *iszData, SLICE *orSlice)
{
	if(strcmp(iszTop, "dynz") == 0)
		extract_dyn_min_max(iszTag, iszData, &orSlice->dynz);

	else if(strcmp(iszTop, "dynv") == 0)
		extract_dyn_min_max(iszTag, iszData, &orSlice->dynv);

	else if(strcmp(iszTop, "dynw") == 0)
		extract_dyn_min_max(iszTag, iszData, &orSlice->dynw);

	else if(strcmp(iszTop, "dynzdr") == 0)
		extract_dyn_min_max(iszTag, iszData, &orSlice->dynzdr);

	else if(strcmp(iszTop, "dynldr") == 0)
		extract_dyn_min_max(iszTag, iszData, &orSlice->dynldr);

	else if(strcmp(iszTop, "dynkdp") == 0)
		extract_dyn_min_max(iszTag, iszData, &orSlice->dynkdp);
}

void extract_dyn_min_max(char *iszTag, char *iszData, DYN *orDyn)
{
	if(strcmp(iszTag, "min") == 0)
		orDyn->min = atof(iszData);
	else if(strcmp(iszTag, "max") == 0)
		orDyn->max = atof(iszData);
}

bool return_boolean(char *iszData)
{
	return strcmp(iszData,"On")==0 ? true : false;
}

bool free_rainbow(RAINBOW *ipRainbow)
{
	int lnNumEle = ipRainbow->xmlHeader.scan.pargroup.numele;
	if(ipRainbow->blobData != NULL)
	{
		for(int i=0; i<lnNumEle; i++)
		{
			int lnNumRays = ipRainbow->blobData[i].lnRay_tot;
			for(int j=0; j<lnNumRays; j++)
				secure_delete_array(ipRainbow->blobData[i].lpRay[j].lpData);
			secure_delete_array(ipRainbow->blobData[i].lpRay);
		}
		secure_delete_array(ipRainbow->blobData);
		secure_delete_array(ipRainbow->xmlHeader.scan.slice);
	}
	secure_delete(ipRainbow);
	return true;
}

bool free_sweep(SWEEP *ipSweep)
{
	if(ipSweep == NULL)
		return false;

	for(int i=0; i<ipSweep->lnRay_tot; i++) {
		secure_delete_array(ipSweep->lpRay[i].lpData);
	}
	secure_delete_array(ipSweep->lpRay);
	secure_delete(ipSweep);

	return true;
}

bool free_cmap(CMAP *ipCmap)
{
	if(ipCmap != NULL) {
		free_rainbow(ipCmap->mrVol);
		secure_delete(ipCmap);
		return true;
	}
	return false;
}
