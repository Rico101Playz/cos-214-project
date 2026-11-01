#include <gtest/gtest.h>
#include <iostream>
#include <cassert>
#include "patterns/execution/ApprovalDecision.h"
#include "patterns/execution/ApprovalRequest.h"
#include "patterns/execution/ApprovalHandler.h"
#include "patterns/execution/AutoApproveHandler.h"
#include "patterns/execution/TimeoutEscalationHandler.h"
#include "patterns/execution/ReviewerHandler.h"
#include "patterns/execution/TechLeadHandler.h"
#include "patterns/execution/ReleaseManagerHandler.h"

TEST(ChainOfResponsibility, Chain) {

    // 1. Build the chain: Auto -> Reviewer -> TechLead -> TimeoutEscalation -> ReleaseManager
    ApprovalHandler* autoHandler = new AutoApproveHandler();
    ApprovalHandler* reviewer = new ReviewerHandler();
    ApprovalHandler* techLead = new TechLeadHandler();
    ApprovalHandler* timeout = new TimeoutEscalationHandler("ReleaseManager");
    ApprovalHandler* releaseMgr = new ReleaseManagerHandler();

    autoHandler->setNext(reviewer);
    reviewer->setNext(techLead);
    techLead->setNext(timeout);
    timeout->setNext(releaseMgr);

    // Test 1: Auto Approval
    ApprovalRequest reqAuto("Auto");
    autoHandler->handle(reqAuto);
    EXPECT_TRUE(reqAuto.isResolved()) << "Chain: AutoApprove resolves an Auto request";
    EXPECT_TRUE(reqAuto.isApproved() && reqAuto.getPendingRole() == "Auto") << "Chain: AutoApprove approves an Auto request on its own";

    // Test 2: TechLead Approval (skips Auto and Reviewer, handled by TechLead)
    ApprovalRequest reqTech("TechLead");
    autoHandler->handle(reqTech);
    EXPECT_TRUE(reqTech.isResolved()) << "Chain: TechLead request is resolved by the TechLead handler";
    EXPECT_EQ(reqTech.getPendingRole(), "TechLead") << "Chain: the answer names the role that approves";

    // Test 3: Timeout Escalation (Timeout escalates to ReleaseManager, who then approves it)
    ApprovalRequest reqTimeout("Timeout");
    autoHandler->handle(reqTimeout);
    EXPECT_TRUE(reqTimeout.isResolved()) << "Chain: Timeout escalates to ReleaseManager, who resolves it";
    EXPECT_EQ(reqTimeout.getPendingRole(), "ReleaseManager") << "Chain: escalation changes the pending role";

    // Test 4: Unhandled role falls off the end of the chain unresolved
    ApprovalRequest reqUnknown("CEO");
    autoHandler->handle(reqUnknown);
    EXPECT_FALSE(reqUnknown.isResolved()) << "Chain: an unhandled role falls off the end unresolved";

    // Clean up memory (deletes the entire chain via ApprovalHandler's destructor)
    delete autoHandler;
}