<%!
    import common.project_utils as project
%><%include file="common.template.sh" />

./$SERVERD_NAME -id $SERVER_BUS_ID -c ../etc/$SERVER_FULL_NAME.conf -p $SERVER_PID_FILE_NAME start & ;

export LD_PRELOAD=;

if [ $? -ne 0 ]; then
	ErrorMsg "start $SERVER_FULL_NAME failed.";
	exit $?;
fi

WaitProcessStarted "$SERVER_PID_FILE_NAME" ;

if [ $? -ne 0 ]; then
	ErrorMsg "start $SERVER_FULL_NAME failed.";
	exit $?;
fi

NoticeMsg "start $SERVER_FULL_NAME done.";

exit 0;