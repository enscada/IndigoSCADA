
?
sparkplug_b/sparkplug_b.protoorg.eclipse.tahu.protobuf"?
Payload
	timestamp (:
metrics (2).org.eclipse.tahu.protobuf.Payload.Metric
seq (
uuid (	
body (?
Template
version (	:
metrics (2).org.eclipse.tahu.protobuf.Payload.MetricI

parameters (25.org.eclipse.tahu.protobuf.Payload.Template.Parameter
template_ref (	
is_definition (?
	Parameter
name (	
type (
	int_value (H 

long_value (H 
float_value (H 
double_value (H 
boolean_value (H 
string_value (	H h
extension_value	 (2M.org.eclipse.tahu.protobuf.Payload.Template.Parameter.ParameterValueExtensionH #
ParameterValueExtension*????B
value*?????
DataSet
num_of_columns (
columns (	
types (<
rows (2..org.eclipse.tahu.protobuf.Payload.DataSet.Row?
DataSetValue
	int_value (H 

long_value (H 
float_value (H 
double_value (H 
boolean_value (H 
string_value (	H h
extension_value (2M.org.eclipse.tahu.protobuf.Payload.DataSet.DataSetValue.DataSetValueExtensionH !
DataSetValueExtension*????B
valueZ
RowI
elements (27.org.eclipse.tahu.protobuf.Payload.DataSet.DataSetValue*????*?????
PropertyValue
type (
is_null (
	int_value (H 

long_value (H 
float_value (H 
double_value (H 
boolean_value (H 
string_value (	H K
propertyset_value	 (2..org.eclipse.tahu.protobuf.Payload.PropertySetH P
propertysets_value
 (22.org.eclipse.tahu.protobuf.Payload.PropertySetListH b
extension_value (2G.org.eclipse.tahu.protobuf.Payload.PropertyValue.PropertyValueExtensionH "
PropertyValueExtension*????B
valueg
PropertySet
keys (	@
values (20.org.eclipse.tahu.protobuf.Payload.PropertyValue*????`
PropertySetListC
propertyset (2..org.eclipse.tahu.protobuf.Payload.PropertySet*?????
MetaData
is_multi_part (
content_type (	
size (
seq (
	file_name (	
	file_type (	
md5 (	
description (	*	?????
Metric
name (	
alias (
	timestamp (
datatype (
is_historical (
is_transient (
is_null (=
metadata (2+.org.eclipse.tahu.protobuf.Payload.MetaDataB

properties	 (2..org.eclipse.tahu.protobuf.Payload.PropertySet
	int_value
 (H 

long_value (H 
float_value (H 
double_value (H 
boolean_value (H 
string_value (	H 
bytes_value (H C
dataset_value (2*.org.eclipse.tahu.protobuf.Payload.DataSetH E
template_value (2+.org.eclipse.tahu.protobuf.Payload.TemplateH Y
extension_value (2>.org.eclipse.tahu.protobuf.Payload.Metric.MetricValueExtensionH  
MetricValueExtension*????B
value*????B,
org.eclipse.tahu.protobufBSparkplugBProto