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

void runChainOfResponsibilityTests() {
    std::cout << "=== Running Chain of Responsibility Tests ===" << std::endl;

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
    std::cout << "[Test 1 - AutoApprove] Resolved: " << (reqAuto.isResolved() ? "PASS" : "FAIL") << std::endl;

    // Test 2: TechLead Approval (skips Auto and Reviewer, handled by TechLead)
    ApprovalRequest reqTech("TechLead");
    autoHandler->handle(reqTech);
    std::cout << "[Test 2 - TechLead] Resolved: " << (reqTech.isResolved() ? "PASS" : "FAIL") << std::endl;

    // Test 3: Timeout Escalation (Timeout escalates to ReleaseManager, who then approves it)
    ApprovalRequest reqTimeout("Timeout");
    autoHandler->handle(reqTimeout);
    std::cout << "[Test 3 - Timeout Escalation] Resolved: " << (reqTimeout.isResolved() ? "PASS" : "FAIL") << std::endl;

    // Test 4: Unhandled role falls off the end of the chain unresolved
    ApprovalRequest reqUnknown("CEO");
    autoHandler->handle(reqUnknown);
    std::cout << "[Test 4 - Unhandled Role] Unresolved as expected: " << (!reqUnknown.isResolved() ? "PASS" : "FAIL") << std::endl;

    // Clean up memory (deletes the entire chain via ApprovalHandler's destructor)
    delete autoHandler;
}