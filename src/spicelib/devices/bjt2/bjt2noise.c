/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Gary W. Ng
Modified: Alan Gillespie
**********/

#include "ngspice.h"
#include "bjt2defs.h"
#include "cktdefs.h"
#include "iferrmsg.h"
#include "noisedef.h"
#include "suffix.h"

/*
 * BJT2noise (mode, operation, firstModel, ckt, data, OnDens)
 *
 *    This routine names and evaluates all of the noise sources
 *    associated with BJT2's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the BJT2's is summed with the variable "OnDens".
 */

extern void   NevalSrc();
extern double Nintegrate();

int
BJT2noise (int mode, int operation, GENmodel *genmodel, CKTcircuit *ckt, 
           Ndata *data, double *OnDens)
{
    BJT2model *firstModel = (BJT2model *) genmodel;
    BJT2model *model;
    BJT2instance *inst;
    char name[N_MXVLNTH];
    double tempOnoise;
    double tempInoise;
    double noizDens[BJT2NSRCS];
    double lnNdens[BJT2NSRCS];
    int error;
    int i;

    /* define the names of the noise sources */

    static char *BJT2nNames[BJT2NSRCS] = {       /* Note that we have to keep the order */
	"_rc",              /* noise due to rc */        /* consistent with the index definitions */
	"_rb",              /* noise due to rb */        /* in BJT2defs.h */
	"_re",              /* noise due to re */
	"_ic",              /* noise due to ic */
	"_ib",              /* noise due to ib */
	"_1overf",          /* flicker (1/f) noise */
	""                  /* total transistor noise */
    };

    for (model=firstModel; model != NULL; model=model->BJT2nextModel) {
	for (inst=model->BJT2instances; inst != NULL; inst=inst->BJT2nextInstance) {
	     if (inst->BJT2owner != ARCHme) continue;
	    
	    switch (operation) {

	    case N_OPEN:

		/* see if we have to to produce a summary report */
		/* if so, name all the noise generators */

		if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
		    switch (mode) {

		    case N_DENS:
			for (i=0; i < BJT2NSRCS; i++) {
			    (void)sprintf(name,"onoise_%s%s",
				inst->BJT2name,BJT2nNames[i]);


			data->namelist = (IFuid *)
				trealloc((char *)data->namelist,
				(data->numPlots + 1)*sizeof(IFuid));
			if (!data->namelist) return(E_NOMEM);
			(*(SPfrontEnd->IFnewUid))(ckt,
			    &(data->namelist[data->numPlots++]),
			    (IFuid)NULL,name,UID_OTHER,(void **)NULL);
				/* we've added one more plot */
			}
			break;

		    case INT_NOIZ:
			for (i=0; i < BJT2NSRCS; i++) {
			    (void)sprintf(name,"onoise_total_%s%s",
				inst->BJT2name,BJT2nNames[i]);

			data->namelist = (IFuid *)
				trealloc((char *)data->namelist,
				(data->numPlots + 1)*sizeof(IFuid));
			if (!data->namelist) return(E_NOMEM);
			(*(SPfrontEnd->IFnewUid))(ckt,
			    &(data->namelist[data->numPlots++]),
			    (IFuid)NULL,name,UID_OTHER,(void **)NULL);
				/* we've added one more plot */

			    (void)sprintf(name,"inoise_total_%s%s",
				inst->BJT2name,BJT2nNames[i]);

data->namelist = (IFuid *)trealloc((char *)data->namelist,(data->numPlots + 1)*sizeof(IFuid));
if (!data->namelist) return(E_NOMEM);
		(*(SPfrontEnd->IFnewUid))(ckt,
			&(data->namelist[data->numPlots++]),
			(IFuid)NULL,name,UID_OTHER,(void **)NULL);
				/* we've added one more plot */
			}
			break;
		    }
		}
		break;

	    case N_CALC:
		switch (mode) {

		case N_DENS:
		    NevalSrc(&noizDens[BJT2RCNOIZ],&lnNdens[BJT2RCNOIZ],
				 ckt,THERMNOISE,inst->BJT2colPrimeNode,inst->BJT2colNode,
				 model->BJT2collectorConduct * inst->BJT2area * inst->BJT2m);

		    NevalSrc(&noizDens[BJT2RBNOIZ],&lnNdens[BJT2RBNOIZ],
				 ckt,THERMNOISE,inst->BJT2basePrimeNode,inst->BJT2baseNode,
				 *(ckt->CKTstate0 + inst->BJT2gx) * inst->BJT2m);

		    NevalSrc(&noizDens[BJT2_RE_NOISE],&lnNdens[BJT2_RE_NOISE],
				 ckt,THERMNOISE,inst->BJT2emitPrimeNode,inst->BJT2emitNode,
				 model->BJT2emitterConduct * inst->BJT2area * inst->BJT2m);

		    NevalSrc(&noizDens[BJT2ICNOIZ],&lnNdens[BJT2ICNOIZ],
			         ckt,SHOTNOISE,inst->BJT2colPrimeNode, inst->BJT2emitPrimeNode,
				 *(ckt->CKTstate0 + inst->BJT2cc) * inst->BJT2m);

		    NevalSrc(&noizDens[BJT2IBNOIZ],&lnNdens[BJT2IBNOIZ],
				 ckt,SHOTNOISE,inst->BJT2basePrimeNode, inst->BJT2emitPrimeNode,
				 *(ckt->CKTstate0 + inst->BJT2cb) * inst->BJT2m);

		    NevalSrc(&noizDens[BJT2FLNOIZ],(double*)NULL,ckt,
				 N_GAIN,inst->BJT2basePrimeNode, inst->BJT2emitPrimeNode,
				 (double)0.0);
		    noizDens[BJT2FLNOIZ] *= inst->BJT2m * model->BJT2fNcoef * 
				 exp(model->BJT2fNexp *
				 log(MAX(fabs(*(ckt->CKTstate0 + inst->BJT2cb)),N_MINLOG))) /
				 data->freq;
		    lnNdens[BJT2FLNOIZ] = 
				 log(MAX(noizDens[BJT2FLNOIZ],N_MINLOG));

		    noizDens[BJT2TOTNOIZ] = noizDens[BJT2RCNOIZ] +
						    noizDens[BJT2RBNOIZ] +
						    noizDens[BJT2_RE_NOISE] +
						    noizDens[BJT2ICNOIZ] +
						    noizDens[BJT2IBNOIZ] +
						    noizDens[BJT2FLNOIZ];
		    lnNdens[BJT2TOTNOIZ] = 
				 log(noizDens[BJT2TOTNOIZ]);

		    *OnDens += noizDens[BJT2TOTNOIZ];

		    if (data->delFreq == 0.0) { 

			/* if we haven't done any previous integration, we need to */
			/* initialize our "history" variables                      */

			for (i=0; i < BJT2NSRCS; i++) {
			    inst->BJT2nVar[LNLSTDENS][i] = lnNdens[i];
			}

			/* clear out our integration variables if it's the first pass */

			if (data->freq == ((NOISEAN*)ckt->CKTcurJob)->NstartFreq) {
			    for (i=0; i < BJT2NSRCS; i++) {
				inst->BJT2nVar[OUTNOIZ][i] = 0.0;
				inst->BJT2nVar[INNOIZ][i] = 0.0;
			    }
			}
		    } else {   /* data->delFreq != 0.0 (we have to integrate) */

/* In order to get the best curve fit, we have to integrate each component separately */

			for (i=0; i < BJT2NSRCS; i++) {
			    if (i != BJT2TOTNOIZ) {
				tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
				      inst->BJT2nVar[LNLSTDENS][i], data);
				tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
				      lnNdens[i] + data->lnGainInv,
				      inst->BJT2nVar[LNLSTDENS][i] + data->lnGainInv,
				      data);
				inst->BJT2nVar[LNLSTDENS][i] = lnNdens[i];
				data->outNoiz += tempOnoise;
				data->inNoise += tempInoise;
				if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
				    inst->BJT2nVar[OUTNOIZ][i] += tempOnoise;
				    inst->BJT2nVar[OUTNOIZ][BJT2TOTNOIZ] += tempOnoise;
				    inst->BJT2nVar[INNOIZ][i] += tempInoise;
				    inst->BJT2nVar[INNOIZ][BJT2TOTNOIZ] += tempInoise;
                                }
			    }
			}
		    }
		    if (data->prtSummary) {
			for (i=0; i < BJT2NSRCS; i++) {     /* print a summary report */
			    data->outpVector[data->outNumber++] = noizDens[i];
			}
		    }
		    break;

		case INT_NOIZ:        /* already calculated, just output */
		    if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
			for (i=0; i < BJT2NSRCS; i++) {
			    data->outpVector[data->outNumber++] = inst->BJT2nVar[OUTNOIZ][i];
			    data->outpVector[data->outNumber++] = inst->BJT2nVar[INNOIZ][i];
			}
		    }    /* if */
		    break;
		}    /* switch (mode) */
		break;

	    case N_CLOSE:
		return (OK);         /* do nothing, the main calling routine will close */
		break;               /* the plots */
	    }    /* switch (operation) */
	}    /* for inst */
    }    /* for model */

return(OK);
}
            

