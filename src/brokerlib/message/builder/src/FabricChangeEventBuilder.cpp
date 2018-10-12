/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/builder/include/FabricChangeEventBuilder.h"
#include "message/payload/include/FabricChangeEventPayload.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"

using namespace dxl::broker::message;
using namespace dxl::broker::message::builder;
using namespace dxl::broker::message::payload;
using namespace std;

/** {@inheritDoc} */
const shared_ptr<DxlMessage> FabricChangeEventBuilder::buildMessage() const
{
    DxlMessageService& messageService = DxlMessageService::getInstance();
    const shared_ptr<DxlEvent> changeEvent = messageService.createEvent();
    changeEvent->setPayload( FabricChangeEventPayload() );
    return changeEvent;
}