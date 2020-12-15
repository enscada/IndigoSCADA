/********************************************************************************
 * Copyright (c) 2014-2019 Cirrus Link Solutions and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Cirrus Link Solutions - initial implementation
 ********************************************************************************/
//apa modified, for compiling in ANSI C
#include <stdio.h>
#include <stdlib.h>
#include "pb_decode.h"
#include "pb_encode.h"
#include "tahu.h"
#include "tahu.pb.h"
#include "malloc.h" //apa+++ for alloca()

// Global sequence variable
uint64_t seq;

/*
 * Private function to decode a Metric from a stream
 */
bool decode_metric(org_eclipse_tahu_protobuf_Payload_Metric *metric, pb_istream_t *stream) {
	bool status;
	pb_istream_t substream;

	pb_wire_type_t metric_wire_type;
	uint32_t metric_tag;
	bool metric_eof;
	const pb_field_t *metric_field;
	uint64_t udest;
	int64_t dest;
	uint32_t dest32;
	float destination_float;
	uint64_t dest64;
	double destination_double;
	pb_byte_t string_size[1];
	pb_byte_t *dest_pb_byte_t;

	if (!pb_make_string_substream(stream, &substream)) {
		fprintf(stderr, "Failed to create the substream for metric decoding\n");
		return false;
	}

	while (pb_decode_tag(&substream, &metric_wire_type, &metric_tag, &metric_eof)) {
		DEBUG_PRINT(("\teof: %s\n", metric_eof ? "true" : "false"));
		DEBUG_PRINT(("\t\tBytes Remaining: %d\n", substream.bytes_left));
		DEBUG_PRINT(("\t\tWiretype: %d\n", metric_wire_type));
		DEBUG_PRINT(("\t\tTag: %d\n", metric_tag));

		if (metric_wire_type == PB_WT_VARINT) {
			DEBUG_PRINT(("\t\tMetric Wire type is PB_WT_VARINT\n"));
			for (metric_field = org_eclipse_tahu_protobuf_Payload_Metric_fields; metric_field->tag != 0; metric_field++) {
				if (metric_field->tag == metric_tag && (((metric_field->type & PB_LTYPE_VARINT) == PB_LTYPE_VARINT) ||
													((metric_field->type & PB_LTYPE_UVARINT) == PB_LTYPE_UVARINT))) {
					DEBUG_PRINT(("\t\tWire type is PB_WT_VARINT\n"));
					
					status = pb_decode_varint(&substream, &udest);
					if (status) {
						DEBUG_PRINT(("\t\tVARINT - Success - new value: %ld\n", udest));
						if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_alias_tag) {
							metric->has_alias = true;
							metric->alias = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_timestamp_tag) {
							metric->has_timestamp = true;
							metric->timestamp = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_datatype_tag) {
							metric->has_datatype = true;
							metric->datatype = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_is_historical_tag) {
							metric->has_is_historical = true;
							metric->is_historical = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_is_transient_tag) {
							metric->has_is_transient = true;
							metric->is_transient = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_is_null_tag) {
							metric->has_is_null = true;
							metric->is_null = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag) {
							metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
							metric->value.int_value = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag) {
							metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
							metric->value.long_value = udest;
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag) {
							metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag;
							metric->value.boolean_value = udest;
						}
					} else {
						fprintf(stderr, "\t\tVARINT - Failed to decode variant!\n");
						return false;
					}
				} else if (metric_field->tag == metric_tag && ((metric_field->type & PB_LTYPE_SVARINT) == PB_LTYPE_SVARINT)) {
					DEBUG_PRINT(("\t\tWire type is PB_WT_SVARINT\n"));
					
					status = pb_decode_svarint(&substream, &dest);
					if (status) {
						DEBUG_PRINT(("\t\tVARINT - Success - new value: %ld\n", dest));
					} else {
						fprintf(stderr, "\t\tVARINT - Failed to decode variant!\n");
						return false;
					}
				}
			}
		} else if (metric_wire_type == PB_WT_32BIT) {
			DEBUG_PRINT(("\t\tMetric Wire type is PB_WT_32BIT\n"));
			for (metric_field = org_eclipse_tahu_protobuf_Payload_Metric_fields; metric_field->tag != 0; metric_field++) {
				if (metric_field->tag == metric_tag && (((metric_field->type & PB_LTYPE_FIXED32) == PB_LTYPE_FIXED32))) {
					DEBUG_PRINT(("\t\tWire type is PB_WT_32BIT\n"));
					
					status = pb_decode_fixed32(&substream, &dest32);
					if (status) {
						DEBUG_PRINT(("\t\t32BIT - Success - new value: %d\n", dest32));
						destination_float = *((float*)&dest32);
						DEBUG_PRINT(("\t\tFLoat - Success - new value: %f\n", destination_float));
						if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_float_value_tag) {
							metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_float_value_tag;
							metric->value.float_value = destination_float;
						}
					}
				}
			}
		} else if (metric_wire_type == PB_WT_64BIT) {
			DEBUG_PRINT(("\t\tMetric Wire type is PB_WT_64BIT\n"));
			for (metric_field = org_eclipse_tahu_protobuf_Payload_Metric_fields; metric_field->tag != 0; metric_field++) {
				if (metric_field->tag == metric_tag && (((metric_field->type & PB_LTYPE_FIXED64) == PB_LTYPE_FIXED64))) {
					DEBUG_PRINT(("\t\tWire type is PB_WT_64BIT\n"));
					
					status = pb_decode_fixed64(&substream, &dest64);
					if (status) {
						DEBUG_PRINT(("\t\t64BIT - Success - new value: %ld\n", dest64));
						destination_double = *((double*)&dest64);
						DEBUG_PRINT(("\t\tDouble - Success - new value: %f\n", destination_double));
						if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_double_value_tag) {
							metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_double_value_tag;
							metric->value.double_value = destination_double;
						}
					}
				}
			}

		} else if (metric_wire_type == PB_WT_STRING) {
			DEBUG_PRINT(("\t\tMetric Wire type is PB_WT_STRING\n"));

			for (metric_field = org_eclipse_tahu_protobuf_Payload_Metric_fields; metric_field->tag != 0; metric_field++) {
				if (metric_field->tag == metric_tag && ((metric_field->type & PB_LTYPE_SUBMESSAGE) == PB_LTYPE_SUBMESSAGE)) {
					DEBUG_PRINT(("\t\tFound a PB_LTYPE_SUBMESSAGE\n"));
				} else if (metric_field->tag == metric_tag &&
							((metric_field->type & PB_LTYPE_FIXED_LENGTH_BYTES) == PB_LTYPE_FIXED_LENGTH_BYTES)) {
					DEBUG_PRINT(("\t\tFound a PB_LTYPE_FIXED_LENGTH_BYTES\n"));
				} else if (metric_field->tag == metric_tag && ((metric_field->type & PB_LTYPE_STRING) == PB_LTYPE_STRING)) {
					DEBUG_PRINT(("\t\tFound a PB_LTYPE_STRING\n"));

					// Get the string size
					
					status = pb_read(&substream, string_size, 1);
					if (status) {
						DEBUG_PRINT(("\t\tString Size: %d\n", string_size[0]));
					} else {
						fprintf(stderr, "\t\tFailed to get the string size while decoding\n");
						return false;
					}

					
					//pb_byte_t dest[string_size[0]+1];
					dest_pb_byte_t = (pb_byte_t *)alloca(string_size[0]+1);
					status = pb_read(&substream, dest_pb_byte_t, string_size[0]);
					if (status) {
						dest_pb_byte_t[string_size[0]] = '\0';

						// This is either the metric name or string value
						if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_name_tag) {
							DEBUG_PRINT(("\t\tRead the Metric name! %s\n", dest_pb_byte_t));
							metric->name = (char *)malloc((strlen(dest_pb_byte_t)+1)*sizeof(char));
							strcpy(metric->name, dest_pb_byte_t);
						} else if (metric_field->tag == org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag) {
							DEBUG_PRINT(("\t\tRead the Metric string_value! %s\n", dest_pb_byte_t));
							metric->which_value = org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag;
							// JPL 04/05/17... I hope this gets FREE(string_value)'d somewhere
							metric->value.string_value =(char *)malloc((strlen(dest_pb_byte_t)+1)*sizeof(char));
							strcpy(metric->value.string_value, dest_pb_byte_t );
							// JPL 04/05/17... local memory?
							//	metric->value.string_value = dest_pb_byte_t;
						}
					} else {
						fprintf(stderr, "\t\tFailed to read the string...\n");
						return false;
					}
					
				} else if (metric_field->tag == metric_tag && ((metric_field->type & PB_LTYPE_BYTES) == PB_LTYPE_BYTES)) {
					DEBUG_PRINT(("\t\tFound a PB_LTYPE_BYTES\n"));
				//} else {
					//DEBUG_PRINT(("\t\tother: %d\n", metric_field->type);
				}
			}

		} else if (metric_wire_type == PB_WT_32BIT) {
			DEBUG_PRINT(("\t\tMetric Wire type is PB_WT_32BIT\n"));
		//} else {
			//DEBUG_PRINT(("\t\tMetric Other? %d\n", metric_wire_type);
		}
	}

	// Close the substream
	pb_close_string_substream(stream, &substream);
}

/*
 * Private function to increase the size of an array of strings
 */
int grow_char_array(char **array, int current_size, int num_new_elems) {
        const int total_size = current_size + num_new_elems;
	char *temp = (char *)realloc(*array, (total_size * sizeof(char*)));

	if (temp == NULL) {
		fprintf(stderr, "Cannot allocate more memory.\n");
		return 0;
	} else {
		*array = temp;
	}

	return total_size;
}

/*
 * Private function to increase the size of an array of Metrics
 */
int grow_metrics_array(org_eclipse_tahu_protobuf_Payload_Metric **metric_array, int current_size, int num_new_elems) {
        const int total_size = current_size + num_new_elems;
        org_eclipse_tahu_protobuf_Payload_Metric *temp = (org_eclipse_tahu_protobuf_Payload_Metric*)realloc(*metric_array,
                                                                 	(total_size * sizeof(org_eclipse_tahu_protobuf_Payload_Metric)));

	if (temp == NULL) {
		fprintf(stderr, "Cannot allocate more memory.\n");
		return 0;
	} else {
		*metric_array = temp;
	}

        return total_size;
}

/*
 * Private function to increase the size of an array of PropertyValues
 */
int grow_propertyvalues_array(org_eclipse_tahu_protobuf_Payload_PropertyValue **values_array, int current_size, int num_new_elems) {
        const int total_size = current_size + num_new_elems;
        org_eclipse_tahu_protobuf_Payload_PropertyValue *temp = (org_eclipse_tahu_protobuf_Payload_PropertyValue*)realloc(*values_array,
                                                                 (total_size * sizeof(org_eclipse_tahu_protobuf_Payload_PropertyValue)));

	if (temp == NULL) {
		fprintf(stderr, "Cannot allocate more memory.\n");
		return 0;
	} else {
		*values_array = temp;
	}

        return total_size;
}

/*
 * Add Metadata to an existing Metric
 */
void add_metadata_to_metric(org_eclipse_tahu_protobuf_Payload_Metric *metric, org_eclipse_tahu_protobuf_Payload_MetaData *metadata) {
	metric->has_metadata = true;
	metric->metadata = *metadata;
}

/*
 * Add a complete Metric to an existing Payload
 */
void add_metric_to_payload(org_eclipse_tahu_protobuf_Payload *payload, org_eclipse_tahu_protobuf_Payload_Metric *metric) {

	int size = payload->metrics_count;
	if (size == 0) {
		payload->metrics = (org_eclipse_tahu_protobuf_Payload_Metric *) calloc(1, sizeof(org_eclipse_tahu_protobuf_Payload_Metric));
		if(payload->metrics == NULL) {
			fprintf(stderr, "Cannot allocate initial memory for data\n");
		} else {
			size = 1;
		}
	} else {
		size = grow_metrics_array(&payload->metrics, size, 1);
	}

	// Assign the metric
	payload->metrics[payload->metrics_count] = *metric;

	// Increment the metric count
	payload->metrics_count++;
}

/*
 * Add a simple Property to an existing PropertySet
 */
bool add_property_to_set(org_eclipse_tahu_protobuf_Payload_PropertySet *propertyset, const char *key, uint32_t datatype, bool is_null, const void *value, size_t size_of_value) {

	int size;

	if(propertyset->keys_count != propertyset->values_count) {
		fprintf(stderr, "Invalid PropertySet!\n");
		return false;
	}

	size = propertyset->keys_count;
	if (size == 0) {
		propertyset->keys = (char **) calloc(1, sizeof(char*));
		propertyset->values = (org_eclipse_tahu_protobuf_Payload_PropertyValue *) calloc(1, sizeof(org_eclipse_tahu_protobuf_Payload_PropertyValue));
		if(propertyset->values == NULL) {
			fprintf(stderr, "Cannot allocate initial memory for data\n");
		} else {
			size = 1;
		}
	} else {
		grow_char_array(propertyset->keys, size, 1);
		size = grow_propertyvalues_array(&propertyset->values, size, 1);
	}

	// Set the key name in the array of keys
	propertyset->keys[size-1] = (char *)malloc((strlen(key)+1)*sizeof(char));
	strcpy(propertyset->keys[size-1], key);

	// Set the value components
	propertyset->values[size-1].has_type = true;
	propertyset->values[size-1].type = datatype;
	propertyset->values[size-1].has_is_null = is_null;
	if (is_null) {
		propertyset->values[size-1].is_null = true;
	}
	if (datatype == PROPERTY_DATA_TYPE_UNKNOWN) {
		fprintf(stderr, "Can't create property value with unknown datatype!\n");
	} else if (datatype == PROPERTY_DATA_TYPE_INT8) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int8_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_int_value_tag;
		propertyset->values[size-1].value.int_value = *((int8_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_INT16) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int16_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_int_value_tag;
		propertyset->values[size-1].value.int_value = *((int16_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_INT32) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int32_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_int_value_tag;
		propertyset->values[size-1].value.int_value = *((int32_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_INT64) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int64_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_long_value_tag;
		propertyset->values[size-1].value.long_value = *((int64_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_UINT8) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %u\n", datatype, *((uint8_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_int_value_tag;
		propertyset->values[size-1].value.int_value = *((uint8_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_UINT16) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %u\n", datatype, *((uint16_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_int_value_tag;
		propertyset->values[size-1].value.int_value = *((uint16_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_UINT32) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %u\n", datatype, *((uint32_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_long_value_tag;
		propertyset->values[size-1].value.long_value = *((uint32_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_UINT64) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((uint64_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_long_value_tag;
		propertyset->values[size-1].value.long_value = *((uint64_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_DATETIME) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((uint64_t *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_long_value_tag;
		propertyset->values[size-1].value.long_value = *((uint64_t *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_FLOAT) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %f\n", datatype, *((float *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_float_value_tag;
		propertyset->values[size-1].value.float_value = *((float *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_DOUBLE) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %f\n", datatype, *((double *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_double_value_tag;
		propertyset->values[size-1].value.double_value = *((double *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_BOOLEAN) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((bool *)value)));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_boolean_value_tag;
		propertyset->values[size-1].value.boolean_value = *((bool *)value);
	} else if (datatype == PROPERTY_DATA_TYPE_STRING || datatype == PROPERTY_DATA_TYPE_TEXT) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %s\n", datatype, (char *)value));
		propertyset->values[size-1].which_value = org_eclipse_tahu_protobuf_Payload_PropertyValue_string_value_tag;
		propertyset->values[size-1].value.string_value = (char *)malloc(size_of_value*sizeof(char));
		strcpy(propertyset->values[size-1].value.string_value, (char *)value);
	} else {
		fprintf(stderr, "Unknown datatype %u\n", datatype);
	}

	propertyset->keys_count++;
	propertyset->values_count++;
	DEBUG_PRINT(("Size of values in PropertySet %d\n", propertyset->keys_count));
}

/*
 * Add a PropertySet to an existing Metric
 */
void add_propertyset_to_metric(org_eclipse_tahu_protobuf_Payload_Metric *metric, org_eclipse_tahu_protobuf_Payload_PropertySet *properties) {
	metric->has_properties = true;
	metric->properties = *properties;
}

/*
 * Add a simple Metric to an existing Payload
 */
void add_simple_metric(org_eclipse_tahu_protobuf_Payload *payload,
			const char *name,
			bool has_alias,
			uint64_t alias,
			uint64_t datatype,
			bool is_historical,
			bool is_transient,
			bool is_null,
			const void *value,
			size_t size_of_value) {

	int size = payload->metrics_count;
	if (size == 0) {
		payload->metrics = (org_eclipse_tahu_protobuf_Payload_Metric *) calloc(1, sizeof(org_eclipse_tahu_protobuf_Payload_Metric));
		if(payload->metrics == NULL) {
			fprintf(stderr, "Cannot allocate initial memory for data\n");
		} else {
			size = 1;
		}
	} else {
		size = grow_metrics_array(&payload->metrics, size, 1);
	}

	if (name == NULL) {
		DEBUG_PRINT(("Name is null"));
		payload->metrics[size-1].name = NULL;
	} else {
		payload->metrics[size-1].name = (char *)malloc((strlen(name)+1)*sizeof(char));
		strcpy(payload->metrics[size-1].name, name);
	}
	payload->metrics[size-1].has_alias = has_alias;
	if (has_alias) {
		payload->metrics[size-1].alias = alias;
	}
	payload->metrics[size-1].has_timestamp = true;
	payload->metrics[size-1].timestamp = get_current_timestamp();
	payload->metrics[size-1].has_datatype = true;
	payload->metrics[size-1].datatype = datatype;
	payload->metrics[size-1].has_is_historical = is_historical;
	if (is_historical) {
		payload->metrics[size-1].is_historical = is_historical;
	}
	payload->metrics[size-1].has_is_transient = is_transient;
	if (is_transient) {
		payload->metrics[size-1].is_transient = is_transient;
	}
	payload->metrics[size-1].has_is_null = is_null;
	if (is_null) {
		payload->metrics[size-1].is_null = is_null;
	}
	payload->metrics[size-1].has_metadata = false;
	payload->metrics[size-1].has_properties = false;

	// Default dynamically allocated members to NULL
	payload->metrics[size-1].value.string_value = NULL;

	DEBUG_PRINT(("Setting datatype and value - value size is %d\n", size_of_value));
	if (datatype == METRIC_DATA_TYPE_UNKNOWN) {
		fprintf(stderr, "Can't create metric with unknown datatype!\n");
	} else if (datatype == METRIC_DATA_TYPE_INT8) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int8_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
		payload->metrics[size-1].value.int_value = *((int8_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_INT16) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int16_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
		payload->metrics[size-1].value.int_value = *((int16_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_INT32) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int32_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
		payload->metrics[size-1].value.int_value = *((int32_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_INT64) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((int64_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
		payload->metrics[size-1].value.long_value = *((int64_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_UINT8) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %u\n", datatype, *((uint8_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
		payload->metrics[size-1].value.int_value = *((uint8_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_UINT16) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %u\n", datatype, *((uint16_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
		payload->metrics[size-1].value.int_value = *((uint16_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_UINT32) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %u\n", datatype, *((uint32_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
		payload->metrics[size-1].value.long_value = *((uint32_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_UINT64) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((uint64_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
		payload->metrics[size-1].value.long_value = *((uint64_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_DATETIME) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((uint64_t *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
		payload->metrics[size-1].value.long_value = *((uint64_t *)value);
	} else if (datatype == METRIC_DATA_TYPE_FLOAT) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %f\n", datatype, *((float *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_float_value_tag;
		payload->metrics[size-1].value.float_value = *((float *)value);
	} else if (datatype == METRIC_DATA_TYPE_DOUBLE) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %f\n", datatype, *((double *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_double_value_tag;
		payload->metrics[size-1].value.double_value = *((double *)value);
	} else if (datatype == METRIC_DATA_TYPE_BOOLEAN) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((bool *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag;
		payload->metrics[size-1].value.boolean_value = *((bool *)value);
	} else if (datatype == METRIC_DATA_TYPE_STRING || datatype == METRIC_DATA_TYPE_TEXT || datatype == METRIC_DATA_TYPE_UUID) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %s\n", datatype, (char *)value));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag;
		payload->metrics[size-1].value.string_value = (char *)malloc(size_of_value*sizeof(char));
		strcpy(payload->metrics[size-1].value.string_value, (char *)value);
	} else if (datatype == METRIC_DATA_TYPE_BYTES) {
		DEBUG_PRINT(("Datatype BYTES - Not yet supported\n"));
	} else if (datatype == METRIC_DATA_TYPE_DATASET) {
		DEBUG_PRINT(("Datatype DATASET - Not yet supported\n"));
	} else if (datatype == METRIC_DATA_TYPE_FILE) {
		DEBUG_PRINT(("Datatype FILE - Not yet supported\n"));
	} else if (datatype == METRIC_DATA_TYPE_TEMPLATE) {
		DEBUG_PRINT(("Setting datatype: %d, with value: %d\n", datatype, *((bool *)value)));
		payload->metrics[size-1].which_value = org_eclipse_tahu_protobuf_Payload_Metric_template_value_tag;
		payload->metrics[size-1].value.template_value = *((org_eclipse_tahu_protobuf_Payload_Template *)value);
	} else {
		DEBUG_PRINT(("Unknown datatype %u\n", datatype));
	}

	payload->metrics_count++;
	DEBUG_PRINT(("Size of metrics payload %d\n", payload->metrics_count));
}

/*
 * Encode a Payload into an array of bytes
 */
size_t encode_payload(uint8_t **buffer, size_t buffer_length, org_eclipse_tahu_protobuf_Payload *payload) {
        size_t message_length;
	bool node_status;

	// Create the stream
	pb_ostream_t node_stream = pb_ostream_from_buffer(*buffer, buffer_length);

	// Encode the payload
	DEBUG_PRINT(("Encoding...\n"));
	node_status = pb_encode(&node_stream, org_eclipse_tahu_protobuf_Payload_fields, payload);
	message_length = node_stream.bytes_written;
	DEBUG_PRINT(("Message length: %d\n", message_length));

        // Error Check
        if (!node_status) {
                fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&node_stream));
                return -1;
        } else {
                DEBUG_PRINT(("Encoding succeeded\n"));
		return message_length;
        }
}

/*
 * Decode an array of bytes into a Payload
 */
bool decode_payload(org_eclipse_tahu_protobuf_Payload *payload, const void *binary_payload, int binary_payloadlen) {

	// Local vars for payload decoding
	bool status;
	pb_wire_type_t payload_wire_type;
	uint32_t payload_tag;
	bool payload_eof;
	const pb_field_t *payload_field;
	uint64_t udest64;
	int64_t dest64;
	pb_byte_t string_size[1];
	pb_byte_t *dest_pb_byte_t;

	pb_istream_t stream = pb_istream_from_buffer(binary_payload, binary_payloadlen);
	DEBUG_PRINT(("Bytes Remaining: %d\n", stream.bytes_left));

	// Loop over blocks while decoding portions of the payload
	while (pb_decode_tag(&stream, &payload_wire_type, &payload_tag, &payload_eof)) {
		DEBUG_PRINT(("payload_eof: %s\n", payload_eof ? "true" : "false"));
		DEBUG_PRINT(("\tBytes Remaining: %d\n", stream.bytes_left));
		DEBUG_PRINT(("\tWiretype: %d\n", payload_wire_type));
		DEBUG_PRINT(("\tTag: %d\n", payload_tag));

		if (payload_wire_type == PB_WT_VARINT) {
			for (payload_field = org_eclipse_tahu_protobuf_Payload_fields; payload_field->tag != 0; payload_field++) {
				if (payload_field->tag == payload_tag && (((payload_field->type & PB_LTYPE_VARINT) == PB_LTYPE_VARINT) ||
										((payload_field->type & PB_LTYPE_UVARINT) == PB_LTYPE_UVARINT))) {
					DEBUG_PRINT(("\tWire type is PB_WT_VARINT\n"));
					
					status = pb_decode_varint(&stream, &udest64);
					if (status) {
						DEBUG_PRINT(("\tVARINT - Success - new value: %ld\n", udest64));
					} else {
						fprintf(stderr, "\tVARINT - Failed to decode variant!\n");
						return false;
					}

					if (payload_field->tag == org_eclipse_tahu_protobuf_Payload_timestamp_tag) {
						payload->has_timestamp = true;
						payload->timestamp = udest64;
					} else if (payload_field->tag == org_eclipse_tahu_protobuf_Payload_seq_tag) {
						payload->has_seq = true;
						payload->seq = udest64;
					}
				} else if (payload_field->tag == payload_tag && ((payload_field->type & PB_LTYPE_SVARINT) == PB_LTYPE_SVARINT)) {
					DEBUG_PRINT(("\tWire type is PB_WT_SVARINT\n"));
					
					status = pb_decode_svarint(&stream, &dest64);
					if (status) {
						DEBUG_PRINT(("\tVARINT - Success - new value: %ld\n", dest64));
					} else {
						fprintf(stderr, "\tVARINT - Failed to decode variant!\n");
						return false;
					}
				}
			}
		} else if (payload_wire_type == PB_WT_64BIT) {
			DEBUG_PRINT(("\tWire type is PB_WT_64BIT\n"));
		} else if (payload_wire_type == PB_WT_STRING) {
			DEBUG_PRINT(("\tWire type is PB_WT_STRING\n"));
			for (payload_field = org_eclipse_tahu_protobuf_Payload_fields; payload_field->tag != 0; payload_field++) {
				if (payload_field->tag == payload_tag && ((payload_field->type & PB_LTYPE_SUBMESSAGE) == PB_LTYPE_SUBMESSAGE)) {
					DEBUG_PRINT(("\tFound a PB_LTYPE_SUBMESSAGE\n"));

					// This is a metric!
					if (payload_field->ptr == NULL) {
						fprintf(stderr, "Invalid field descriptor\n");
						return false;
					}

					{
					org_eclipse_tahu_protobuf_Payload_Metric metric = org_eclipse_tahu_protobuf_Payload_Metric_init_zero;
					if(decode_metric(&metric, &stream)) {
						DEBUG_PRINT(("Decoding metric succeeded\n"));
						add_metric_to_payload(payload, &metric);
					} else {
						fprintf(stderr, "Decoding metric failed\n");
						return false;
					}
					}
				} else if (payload_field->tag == payload_tag && ((payload_field->type & PB_LTYPE_FIXED_LENGTH_BYTES) == PB_LTYPE_FIXED_LENGTH_BYTES)) {
					DEBUG_PRINT(("\tFound a PB_LTYPE_FIXED_LENGTH_BYTES\n"));
				} else if (payload_field->tag == payload_tag && ((payload_field->type & PB_LTYPE_STRING) == PB_LTYPE_STRING)) {
					DEBUG_PRINT(("\tFound a PB_LTYPE_STRING\n"));

					// Get the UUID
					//pb_byte_t string_size[1];
					status = pb_read(&stream, string_size, 1);
					if (status) {
						DEBUG_PRINT(("\t\tUUID Size: %d\n", string_size[0]));
					} else {
						fprintf(stderr, "\t\tFailed to read the UUID\n");
						return false;
					}

					//pb_byte_t dest[string_size[0]+1];
					dest_pb_byte_t = (pb_byte_t *)alloca(string_size[0]+1);
					status = pb_read(&stream, dest_pb_byte_t, string_size[0]);
					if (status) {
						dest_pb_byte_t[string_size[0]] = '\0';
						DEBUG_PRINT(("\t\tRead the UUID: %s\n", dest_pb_byte_t));
						payload->uuid = (char *)malloc((strlen(dest_pb_byte_t)+1)*sizeof(char));;
						strcpy(payload->uuid, dest_pb_byte_t);
					} else {
						fprintf(stderr, "\t\tFailed to read the UUID...\n");
						return false;
					}

				} else if (payload_field->tag == payload_tag && ((payload_field->type & PB_LTYPE_BYTES) == PB_LTYPE_BYTES)) {
					DEBUG_PRINT(("\tFound a PB_LTYPE_BYTES\n"));
				//} else {
					//DEBUG_PRINT(("\tother: %d\n", payload_field->type);
				}
			}
		} else if (payload_wire_type == PB_WT_32BIT) {
			DEBUG_PRINT(("\tWire type is PB_WT_32BIT\n"));
		} else {
			fprintf(stderr, "\tUnknown wiretype...\n");
		}
	}

#ifdef SPARKPLUG_DEBUG
	// Print the message data
	print_payload(payload);
#endif

	return true;
}

/*
 * Free memory from an existing Payload
 */
void free_payload(org_eclipse_tahu_protobuf_Payload *payload) {
	int i=0;
	for (i=0; i<payload->metrics_count; i++) {
		free(payload->metrics[i].name);
		// More TODO...
		// JPL 04/05/17... free up string data allocated memory
		if(  payload->metrics[i].which_value == 
			 org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag ) // 15 ???
		{
		  if(payload->metrics[i].value.string_value)  // not null?
		  {
			free(payload->metrics[i].value.string_value);
		  }
		}
		
		if (payload->metrics[i].has_properties)
		{
			int j=0;
			for (j=0; j<payload->metrics[i].properties.keys_count; j++){
				free(payload->metrics[i].properties.keys[j]);
								
				if(payload->metrics[i].properties.values[j].which_value ==
					org_eclipse_tahu_protobuf_Payload_PropertyValue_string_value_tag) 
				{
					if(payload->metrics[i].properties.values[j].value.string_value)
					{	
						free(payload->metrics[i].properties.values[j].value.string_value);
					}
				}
			}
			free(payload->metrics[i].properties.keys);
			free(payload->metrics[i].properties.values);	
		}	
		
	}
}

#include <windows.h>
/*
 * Get the current timestamp in milliseconds
 */
uint64_t get_current_timestamp() {

#ifdef UNIX
	// Set the timestamp
	struct timespec ts;
	#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		ts.tv_sec = mts.tv_sec;
		ts.tv_nsec = mts.tv_nsec;
	#else
		clock_gettime(CLOCK_REALTIME, &ts);
	#endif
	return ts.tv_sec * UINT64_C(1000) + ts.tv_nsec / 1000000;

#else //UNIX
	FILETIME ft;
	��tG�ut�;P�ay Y�TG�M�D�t$�����}� �U�E�tf�Qf�U�U�f�If�M�M�9Utf�} v�U�+�JQPV�� ���Ef�M+�f�P��T����PS��� �}�+u����7w��`G �g jX�M�_^[d�    �� ���I u���I �    h�@ �@t Y�V��j �Ƨ  �D$Y�N�@�RP�@� ���    jX^� ���F �Cx ��   ��iI S�E�3��E��E�E�V�E�W�}P�E��P�E�P�G��^RP���� �u�E���P�� �@�e� ��u�PfG jP�M���  �M��E��V����`G ���   �E+=�  vh�  �u��u��j� �E����G�˙RP�w��\����u�P�.� �@�E���u�PfG j�M�^VP�[  ��\����E��3V��h�  �u��u��� �u�蛦  ����tj^�M��E��  �M���M��  �M��_^[d�    �� jX� V�5�iI Wjd�Q�  �$�  ��GI VW觔 h�  VW蛔 ��jX_^� ��F ��v V�u��  ��u��eG P��cG �E��$  �e� �EP�  �u����cG �M��^d�    �� ��F �ov ��(  SV�u����  ��tj �u�u� aG �o  W3���  3�������f�������f���������P�u��  ���������   ��������u��GI P��������E���   ;�u��GI P����j,�*w Y�E;Ɖu�t���Y���� �MjSP��   �3���$  �M���E�u�PVSh}�@ VV��`G ��j�}���v Y�8�=�dG h�   j�VP�Ej�׃��EuBjVV�E�VP��dG ��t �}�2�  u��$  ��  �E�P��dG ��h�   j�V�uj��u�W� ��$  Y;�_t�K�h  S�=� Y�}�u3��!�E�u�P�u���`G �u��8cG 3�9u����M�^[d�    �� �D$V��Wj �t$�& �~�F���G�fG �G�fG �8  �D$��fG ��
�F$�F(��_^� V���   �D$tV西 Y��^� �  SV�t$2�V�cG ��~f�>\u	f�~\u�Ê�^[� U���S�]V3�jVS��z ���uh,�H S��dG ��f93��tf�8\uf� / @@f90u�S�cG ��~eW3��}�f�u쫫��f���}쥥�E�h�H P��(� YY_��tOf�Ch�H f�E��E�P�� Y��Yt3f�Ch��H f�E��E�P�� Y��Yt�uh�H h�H S��dG ��^[�� �>�F �Ys Q�} V��W�u�t�F�fG �F�fG �N�F�y��@cG ��u�e� ��cG �Fj�E��at �F�F   ��fG �FY�@�t0�DcG �M���fG ��_^d�    �� V���v��fG ��� �D$YtV�� Y��^� ��@�4�DcG �SUV��W�F�^��fG �@�|0�@cG �n��u ��cG ��e  �@�t0�DcG �ƍN���#���y��@cG ��v��fG �s� ��Y���#��0��cG ���#��@�40�DcG _^][ø^�F �r Q�} V��W�u�t�F�fG �F�fG �N�F�y��@cG ��u�e� ��cG �Fj�E��s �F�F   ��fG �FY�@�t0�DcG �M���fG ��_^d�    �� SUV��W�F�^��fG �@�|0�@cG �n��u ��cG ��e  �@�t0�DcG �ƍN���#���y��@cG ��v��fG �T� ��Y���#��0��cG ���#��@�40�DcG _^][�V���G����D$tV�� Y��^� V���J����D$tV��� Y��^� ���I u���I �    h�@ �_l Y�V�5@cG W3��D$�p��bG ��u
����G��d|�_3�^�V����t�   �& ^�SV��W�vD臻 �vH3ۉ^D�z� �vL�^H�o� ���   ���   ��;ˉ^Lt��P����   ���   �A� Y������ _^[��t$�   � �p�F ��o ���EV��F(�E�P�  �e� �ȃ����#�Q�M�%   �M���M��	����u���B   �M�^d�    �� V��W�|$�;t��tP��cG �7��cG P�7��cG ���_^� ���F �To ��  �v SV��3�W�u�9t
��� ��  �M��]��]��]ȉ]̉]Љ]��u	  �  3������f������Sh�H j�NS�]�f��V  ����H tSWjS�N�@  ��u�v�����hH�H P��dG ��SWjS�N�  ������E�t?h�   ��o ��Y�}�;��E�tSSSSh0u  h4�H ���7  ��fG �3��]��}��5h�   �o Y�E�;��E�tSSSSh0u  h4�H ����  �3��]��E�F�}䍍���h @�QP���<� ��u6�=@cG �ׅ��׋���׋�����  ��  ��N�}��U��R�P��   ����P�N�Eԋ��U��R�8]�u;�v���u�T�����ES�M��E��fG � �E��fG P�.�����fG �u����E��e��SP���E��G�fG �G�fG �  �M��7��  ;�uT�5@cG �օ��֋���֋�����  ��  ��E�u��U��HR��P�M��]��u������M���M��  ���i  �E���d���P�I  �   �E�V�]�]�]��,n �=@cG Y�E܉]�8]��   8]���   �M�E�PV�u��D� �}���E8]uU9]�v&�E�SP�u��u��u��cG ��t�E�����;��E8]u%�E�U�EЋE�R�H��P���E�9]��E��2�ׅ������%��    ��E��E��E�U�R�H��P���E8]��A����u��@� �}��Yt�8cG ;�t	�u��ЃM���M��P8]�u8]u��d����S  �E�U�R�H��P��d����E��:  �M��]������M���M��<  �E��M�_^d�    [�� V�q$V�aG �^�SVW���_$�w$V�aG �6��u	���� ���u��t	�j���P_�C�^[ø��F �4k ��SV��W�u��u  �~03ۋω]��  �E��^D�^H�^L�^P�^T�^\�^`�^d�^h�^l�^p�^t�Fx   ǆ�      ��dG ���   j0���   ���   ���   ���   �l Y�ȉM�;��E�t��   �3����   �E�F,�E   ��P�E��u���   �}G �u�u�u�� �E�P�aG H��������   t�E䉆�   �E艆�   �ǆ�   �  ���   �M��_^[d�    �� V��j�f �Vk ��Yt�    �3��N�F�� �f �}G ��^��}G ��� V���}G ��� �D$tV�� Y��^� ���F �i QS�aG VW3���WWWW�u��~�~�ӉFWWWW�}��ӉF�N�E��+� �M�~�~�~ �~(�F$   �F,0u  �L}G ��_^[d�    �ø�F �2i QV��W�u��L}G �v�E�   �O� �F�=8cG ��Yt��tP�׃f �F��t��tP�׃f �M�_^d�    ��2�� V�������D$tV��� Y��^� ð�V���   �D$tV�ܳ Y��^� �3�F �h QV��u��}G �E�   �����e� �N0�}G �}� �M���������M�^d�    �øR�F �Dh QQSV��W�u��,  �~03ۋω]����   ��|G �   ����   �h�I ;�u)j�Ri Y�ȉM�;��E�t������3��Ȉ]��h�I 9Yu �E   P�u�u�u�u�� �h�I ��;�t����� S���{   hl�I �aG Ɔ�   ��E��   P�u�u�u�u��� �E�M�F,��_^[d�    �� U����e� W3��}���E��E�   P�E�Pj(j ��� �E���_�ËD$�P�Q�P�Q�@P�A�aG � �y�F ��f ��SV��u��A���3ۍN0�]�������E��^D�^H�^L�^P�^T�^\�^`�^d�^h�^l�^p�^t�Fx   ǆ�      ��dG ���   j0���   ���   ���   ���   ��g Y�ȉM�;��E�t�����3����   �E�P���   �}G �aG ����:È��   t�E䉆�   �E艆�   �ǆ�   �  ���   �M��^[d�    ��V���   �D$tV�.� Y��^� ��|G �G������� �A(��t�L$��� V���   �D$tV�� Y��^� ��fG �������F �e QVW���}����e� �wj ���F�fG �F�fG �  �M����fG _^d�    �ø��F �Pe QSUVW��d$ �-8cG �D$   �~�D$(��P��  �Gj h�   jj jh   @P�$cG �؋;�t���t
��tP�Ճ���>�u�|$h�  ��bG �D$뢃>����L$��L$(�������L$��_^][d�    �� Vj �t$���F�fG �F�fG �  ��fG �F��^� �a �V��~ tV�����Y�������^ø��F �Ud QV��u��e� �N��������t�8cG ��tQ�Ѓ��M�^d�    �ø��F �d ��\  SV������W3�P��h  �]���aG �E��NP�M���   �@������QSP������P�aG �M��"���������SP�M��EܘfG �E�fG �B�����fG �}�8^(�E�   t`�E��M�P�   �p�5aG ������P�֍M�������M��E�SP�  �p������P�֍M������u���cG ������P��cG �E��u�E�SP���F�fG �F�fG �8  �>�E�   �MЈ]��_����M��_^[d�    �� U���,VW�E�3���P�}��]   �@;�t	P��cG ���M�������fG ��P�E�jP�E�.   �  ��fG ;�t;�r+�jP�jQ�MWV�  �E_^�� ��F �nb ��LSVW�=�fG j�M�^3�Vh`�H �MЉ]�������M�WP�u��  �MЋ��]������M��E�SP��   ;=�fG �E�   t&9]�t�u���cG ;�r�MGVWS�u��y  �u���M�E�VP��  �u�M��]��1����M�E_^[d�    �� U���,SV�E�W3ۋ�P�]��-����@�=�cG ;�tP�׋؍M��������fG ��P�E�jP�E�.   �^  ��fG ;�t;�r�} t@jQ��F��tP��j�5�fG �MPV��  �E_^[�� �8�F �'a ��@SV��3�W�u��]���   ��tej�`�H [�M�SW�����e� jP���  �M���MЋ��?���;5�fG t&SW�M��k����M�FVP�]���  �M���MЋ�����SVj �u��x���   ������}�ujS�;�ju�Ff�@f-: f���$�@@PS�C[�M�Sh`�H ����WP���E�   �n  �M���M��������E��u+}����S��#E�WPV�M�   �M�E_^[d�    �� V��F��t!P��cG ��r�Ff�8\uf�x\ujX^�3�^�VW3���Wh�H jW�<  ��ujX�MWh�H jW���#  ��uj��Wh��H jW���  ��uj��Whh�H jW����  ��t�3�9~��H_^øT�F �o_ �� �} V��u�t�FgG �FgG j ����   �U�E�M�e� R�FP�E�P�V�N � gG �,  P���E���  �e� �M��i����M��^d�    �� �S����|$ VW�|$��t%���FgG �FgG u3��
�G�@�D8� �F$j W����   �G�F�G�F�G �F � gG ��_^� V�������D$tV輩 Y��^� V����t�8cG ��tQ�Ѓ& ^øv�F �W^ Q�} V��W�u�t�F�fG �F�fG �N�F�y��@cG ��e� �f j�E��g_ �F�F   ��fG �FY�@�t0�DcG �M���fG ��_^d�    �� ���F ��] Q�} SV��W�u�t�F�fG �F�fG �F�^�x��@cG ��e� �f j�E�_W��^ �F�~��fG �F�=DcG Y�@�t0�׋E��fG �p��cG P�E�p��cG �F��@�t0�׋M��_^[d�    �� V��W�|$�F;Gt!��tP��cG �w��cG P�w��cG �F��_^� U��Q�e� V��F��tP��cG �U;�fG u�M��MW�<;�_v+��ЋFj�H�MPR�   �E^�� ���F �\ Q�} V��W�u�t�F�fG �F�fG �N�F�y��@cG ��u�e� �u��cG �Fj�E��] �F�F   ��fG �FY�@�t0�DcG �M���fG ��_^d�    �� U��VW��� tI�G��tP��cG �M��fG ;�t;�s��;Mr2+M��x�u�G�u�pP�4  ����tNy��fG _^]� ��������VW��� t@�t$;5�fG u�G��tP��cG �p���|�G�L$j j�pP�   ;�fG uNy��fG _^� ����U��QSVW��� tK�G��u!E��
P��cG �E��E�]���;E�w&�E�GS�u�pP�x  ����tF�E�E;E�vݡ�fG _^[�� ����SV��3�W9^t;�F�|$;�t	P��cG ��;�}#�F�L$j j�xP�^���;�fG uG�݋����fG _^[� U��QQS�]W��3�;�u3�9O����   9Ou�����   f9�Ct
f�@@f��u�+�V��H�E����G+u;�u3��P��cG �M+��E�E;Es�E�E;�s��P�E�CP�G�HP�u   ���E���ua�E+ƃ���E�u�E��E��H�;�rEf�|C� �7��u8�G��tP��cG �M�U�H;�r�G��tP��cG �Of�|A� u�e� �E�^_[�� U��SVW�   ��u�u�u�u�uPh   � aG �   �u�= cG 3�9]u�]�+�D6��$��` ��S�L6SQPV�E�u�SS�׋E�E9]t(�D6��$��W` ��S�L6SQPV�E�u�SS�׋]VSV�ujh   �aG �e�HH_^[]�U���  �=0HI  u3������ǅ����  P�$aG ��t3����������,HI �0HI �,HI �Ë��L$��  �L$��  � gG � V���   �D$tV薣 Y��^� �gG ø��F �DX V���u�FP��bG ��  �Ej �e� jVh��@ �M�uh  �    ���^t	��tjX�3��M�d�    �� ø��F ��W ��\�EV��u��x u�x t�E��   �j�u�M��p �����e� �E��   �x�H �E�(fG ���E� fG ��u��GI SW�Mj QP�M��#O���EȻPfG ���E��]t�E�f�AHP�7  ����E�P�   �@�E���u��j�M��uWP�����M��E���5���e� �M���5��_[�f�AHP�3  �u�E��M��u�p �u�u����PV��dG �M���M��������ƋM�^d�    �� U��Qf�AH�e� P�u�   �E�� �X�F �V ���   �e� SVW�M�E�(fG � fG j P�M��}��u��M���Ej[P�E�h̪H P�]���  �E�h��H P�����P�  ���E�M썅d���P�E��z  j ��<���j Q���E��P���u�M��E�Q���  ��<����E���4����d����E��4��������E��4����(�x�H �̋Ѕ҉e�9�q u��GI �Uj RP�VM����(�E�̉e�SPh��H �E�	�=����(�E�̉e�SPh��H �E�
�=����(�E��̉e�SP�E�蛉���u�E��Z �Ĥ   �]�M��]��4���e� �M��4���M�E_^d�    [�� ���F �U ���   SV3�W��E�SP�M��E�(fG �E� fG �K���EP�E�h̪H P�]��z
  �E�h��H P�����P�  ������l�����P�E���   S��D���SQ���E��O���M�WQ���E��	  ��D����E��J3����l����E��;3��������E��,3��j�E��(�̉e�jPhܪH �c<����(�E�̉e�jPh��H �E��G<����(�E��̉e�jP�E�	�I����E�� ��|�M����]���2���M���M��2���M�f��_^d�    [�� U��Q�EV�qtP�E���pP�  3�9u�^���� U��Q�e� V�u��  j Q���(fG �F  fG �J����^�� ���F �oS ��TS3ۍE�SP�M��E�(fG �E� fG ��I���E�]�HH�  -  t6H�  �EHt
��eu�An j�u��dG �M���M���1��jX��   W�}h4HI W��o �EY�8HI Y��  �E�E�SP�M�E���   �@;�u�PfG VPW��dG �M��1���54HI �  �]�VW�vp �8HI V��WP�Op �5�dG ��jW��jfW�E�֋8HI ^S9�  t�u��dG �54HI h�  jf�P��dG �54HI h  jW�%p �M�����M���0��jX_��54HI ��`G �M���M���0��3��M�[d�    �� �!�F ��Q ��   SV��t���W3ۋP�]��o  j�E�_�M�SP�}��E�(fG �E� fG �XH��8]�E�u=9]�t8��|���;�u�PfG �SSP�E�h_  P�  P�M��E���0���E��M��$��E�h  P��   P�M��E��0���E��M��0��9]�t5�E�SP����  SP�M��E���c��;0fG �M��E��E��/��8]t_��L���WP���  P�E�hp�H P�E���������50fG �ȃ��E����S#�Q�M������M��E��/����L����E��/���u�E�SP���(fG �F  fG �G���}�M��E��T/����t����]��F/���M��_^[d�    �� ���F �QP ��  SV3�W�M�E�(fG � fG SP�M��]܉}��u���F���E��E�   �@HP�E�h̪H P�  ���E덍\�����\���SP��|����F���u��\����E�h�H P�x  �E�h��H P������P�
  ���E�M썅����P�E������S�����SQ���E��J���u䍍4����E�Q���  ������E�	�A.���������E��2.���������E��#.���������ݐ �E�SP�M��E�
�}��u���E��S�E��(�E��̉e�jPh�H �57����(��4����̉e�jP�E��4����������E��ړ :�t{��(�E�̉e�jPh�GI �H����(��\����̉e�jP�E�������(�E��̉e�jP�E��؂��������������P�E��o� P�M��E���-���������E��<-��9]�u�E��p f�@HP�E��uP�I ���M�E�SP�9�q �QE���E�   �M��E�
��,���������E�蔒 ��4����E���,����\����E���,���M��]���,���M�E_^d�    [�� ���F ��M ��XSVW�E��e��u3�P�]��U���9]�j_�}�u)�u�ESPh�GI ���(fG �F  fG �5E���}���   �ESP�M��E�(fG �E� fG �
D���U�E�;�u#9]��   9]��   �E��M�P�,���   �M;�u%9]u�E�;�u�PfG RP�E�P�  ���Z;�tV9]u�E�;�u�PfG QRP�E�P�  ���4;�t09]t+�E�;�u�PfG �uQRP�E�P�j  ������@ �j3�_�u�E�SP���E�   �(fG �F  fG �C���Mĉ}��E��W+���M��]��L+���M��_^d�    [�� U��Q�u�e� �   �E�� ��F �@L ��0�e� S��VW�{Du'�u�è  j S���(fG �F  fG �,C�����   ��(��H  �e�̿(fG � fG j P�9�q � C���E�P�Q> ��,3��E�   9E�u-�M�è  PS�9�q ��B���e� �M��E�   �x*���E�)�]P�Eċ�P�;�s �B���e� �M��E�   �L*���ËM�_^d�    [�� U���3��E�I8E�E��E�n�E�s�E�t�E�a�E�l�E�l�E�S�E�h�E�i�E�e�E�l�E�d�E�t�E� �E�W�E�i�E�z�E�a�E�r�E�d�E��M�EjP�E�P�D���E�� �C�F ��J ��0�e� SVWj �(fG � fG Q�Mĉ]ĉ}���A��jXj\�E���(�̉e�P�u��~���M��N   �u�E�j P�Ή�~ �A���E�   �e� �M��B)���M��_^d�    [�� �L$�D$P�t$��   ø`�F �:J ��,SV��3�9]�]�t`9^u�EP�m)���PS�M�u0�r���E�FH��P�u0�r��8]�uI:�u�u0�Nj��\���50fG �E����M#�SP�N�����M���M�(���M�^[d�    ��, :�t��50fG �EȍMjP��C���50fG �ȃ��E����S#�Q�N踴���MȈ]��C(��렸t�F �_I �� �E�e� SVW�}�M�E�f�? ��  f�?%��  j[�f�f=% ��  3�f;Ɖu�tHf=# u]��-f=* u�E�E�@��E��f=- tf=+ tf=0 tf=  uf��f;�u�9u�u!W�Z Y�E�f�f��tP�PY ��Yt���3�f�?.uGGf�?*u0�EG�EG�X��e� jh�H W�X ����u,���E�   �MW�Y Y��f�f��t�P��X ��Yt�GG�����Ft#��tHHt��t��u�E�   ��E�   GG�E�c  ;���   ��   ��C��   jY+���   +���   +�tw-��  ��   +���   ���i�  �7  ��G��   ���%  ���  H��   H��   H��   �  -s  t6-��  t&��t
��t��u��E�E�@���t P�G Y�#�Ej^�,�E�E�@���uj^�P�<cG ����}j^���P�����t;�|��;u���   �u��   ��>  9E�~�E����$��6N �E��EQQ���@��$S�u�hةH V�}V V��F ���2�E��   ���nt2Ht#Ht��t��t�$�Ej ^�E��;�|����E�t�E��Eu���E�GG�&����u�M�E�P��.����e� �@��>���u�� �uP�U �M�����M��/���M�e�d�    _^[�� U��QVW�}��W�+   �v�E;�tf�f;Hr�E��u��E���E_^��� �A�$HI �H;�tV�t$f�6f9qs�I����	;�u�^� ���F �E ��T�e� SVWj �(fG �u� fG �Mȉ]ȉ}��<��j^�u�u��E YP�M��u��   VP�M��I   �u�E�j P���E���~ �d<���E�   �M��E��$���e� �M���#���M��_^[d�    �ø��F �E QSV3ۋ�9]W�u�t�HfG �F @fG ��x��@cG ��E�~S�ϊ �]���]>���50fG ��S�u�.X���^�^�^�F �@�t0 �DcG �M��_^[d�    �� �0fG S�\$V��W+F;�w蹏 ��v8��j ~��W�ka����t%�F�V�L$�Bf�f�@@AAKu�F�~f�$x ��_^[� ���I u���I �    h�@ ��? Y�V��hh�H �(aG ���^Ë��tP�,aG ø��F ��C ���SV3�W;Ɖe��u�