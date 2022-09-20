/*

Generated with CMSIS-DSP Compute Graph Scripts.
The generated code is not covered by CMSIS-DSP license.

The support classes and code is covered by CMSIS-DSP license.

*/

{% if config.dumpFIFO %}
#define DEBUGSCHED 1
{% endif %}

#include "arm_math.h"
#include "{{config.customCName}}"
#include "GenericNodes.h"
#include "{{config.appNodesCName}}"
#include "{{config.schedulerCFileName}}.h"



{% macro optionalargs() -%}
{% if config.cOptionalArgs %},{{config.cOptionalArgs}}{% endif %}
{% endmacro -%}

{% block schedArray %}
{% endblock %}

/***********

FIFO buffers

************/
{% for fifo in fifos %}
#define FIFOSIZE{{fifo.fifoID}} {{fifo.length}}
{% endfor %}

{% for buf in sched._graph._allBuffers %}
#define BUFFERSIZE{{buf._bufferID}} {{buf._length}}
{{buf._theType.ctype}} {{config.prefix}}buf{{buf._bufferID}}[BUFFERSIZE{{buf._bufferID}}]={0};

{% endfor %}

uint32_t {{config.schedName}}(int *error{{optionalargs()}})
{
    int cgStaticError=0;
    uint32_t nbSchedule=0;
{% if config.debug %}
    int32_t debugCounter={{config.debugLimit}};
{% endif %}

    /*
    Create FIFOs objects
    */
{% for id in range(nbFifos) %}
{% if fifos[id].hasDelay %}
    FIFO<{{fifos[id].theType.ctype}},FIFOSIZE{{id}},{{fifos[id].isArrayAsInt}}> fifo{{id}}({{config.prefix}}buf{{fifos[id].buffer._bufferID}},{{fifos[id].delay}});
{% else %}
    FIFO<{{fifos[id].theType.ctype}},FIFOSIZE{{id}},{{fifos[id].isArrayAsInt}}> fifo{{id}}({{config.prefix}}buf{{fifos[id].buffer._bufferID}});
{% endif %}
{% endfor %}

    /* 
    Create node objects
    */
{% for node in nodes %}
{% if node.hasState %}
    {{node.typeName}}<{{node.ioTemplate()}}> {{node.nodeName}}({{node.args}});
{% endif %}
{% endfor %}

    /* Run several schedule iterations */
{% block scheduleLoop %}
{% endblock %}
errorHandling:
    *error=cgStaticError;
    return(nbSchedule);
}