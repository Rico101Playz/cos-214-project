#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>
#include <string>

#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/structural/LeafWorkItem.h"
#include "patterns/structural/SecuredTividy.h"
#include "patterns/structural/Tividy.h"
#include "server/DemoSeed.h"

namespace {

template <typename F>
bool denied(F f) {
    try { f(); } catch (const AccessDenied&) { return true; } catch (...) { return false; }
    return false;
}

template <typename F>
bool allowed(F f) {
    try { f(); } catch (...) { return false; }
    return true;
}

TEST(Proxy, Roles) {
    Tividy engine("Acme");
    DemoSeed::company(engine);
    DemoSeed::templates(engine);

    SecuredTividy maya(engine, "Maya");
    SecuredTividy tom(engine, "Tom");
    SecuredTividy bob(engine, "Bob");
    SecuredTividy fay(engine, "Fay");
    EXPECT_TRUE(maya.isManager() && !bob.isManager()) << "Proxy: knows who is a manager";
    bool ghostRefused = false;
    try { SecuredTividy ghost(engine, "Ghost"); } catch (const std::invalid_argument&) { ghostRefused = true; }
    EXPECT_TRUE(ghostRefused) << "Proxy: an unknown person cannot get a proxy";

    EXPECT_THROW(([&] { bob.startProject("Feature Delivery", "x", ""); })(), AccessDenied) << "Proxy: workers cannot start projects";
    std::string p = maya.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    EXPECT_FALSE(p.empty()) << "Proxy: managers can start projects";

    TaskSpec spec;
    spec.name = "Sneaky";
    EXPECT_THROW(([&] { bob.addTask(p, spec); })(), AccessDenied) << "Proxy: workers cannot create tasks";
    EXPECT_THROW(([&] { bob.undo(); })(), AccessDenied) << "Proxy: workers cannot undo";
    EXPECT_THROW(([&] { bob.cancel(p, "no"); })(), AccessDenied) << "Proxy: workers cannot cancel projects";
    EXPECT_THROW(([&] { bob.addUnit("", "x", "X"); })(), AccessDenied) << "Proxy: workers cannot change the company";
    EXPECT_THROW(([&] { bob.addPerson("Eve", "Developer", "backend"); })(), AccessDenied) << "Proxy: workers cannot add people";
    EXPECT_THROW(([&] { bob.moveMember("frontend", "Ben"); })(), AccessDenied) << "Proxy: workers cannot move people";

    EXPECT_THROW(([&] { bob.assign(p, "spec", "Bob"); })(), AccessDenied) << "Proxy: Bob cannot claim work for another role";
    EXPECT_THROW(([&] { tom.assign(p, "spec", "Bob"); })(), AccessDenied) << "Proxy: a worker cannot assign others";
    EXPECT_NO_THROW(([&] { tom.assign(p, "spec", "Tom"); })()) << "Proxy: Tom can claim the TechLead item";
    EXPECT_THROW(([&] { bob.start(p, "spec"); })(), AccessDenied) << "Proxy: only the assignee can start work";
    EXPECT_NO_THROW(([&] { tom.start(p, "spec"); tom.complete(p, "spec"); })()) << "Proxy: the assignee can start and finish";
    EXPECT_THROW(([&] { tom.approve(p, "spec"); })(), AccessDenied) << "Proxy: nobody approves their own work";
    EXPECT_THROW(([&] { bob.approve(p, "spec"); })(), AccessDenied) << "Proxy: a developer cannot approve a Manager approval";
    EXPECT_THROW(([&] { bob.reject(p, "spec", "no"); })(), AccessDenied) << "Proxy: or request changes on it";
    EXPECT_NO_THROW(([&] { maya.approve(p, "spec"); })()) << "Proxy: the manager can approve";

    EXPECT_FALSE(fay.mayClaim(p, "ui")) << "Proxy: blocked work cannot be claimed";
    EXPECT_FALSE(fay.mayClaim(p, "api")) << "Proxy: Fay is not in Back-end, so cannot claim api";
    EXPECT_NO_THROW(([&] { bob.assign(p, "api", "Bob"); })()) << "Proxy: Bob can claim back-end work";
    EXPECT_NO_THROW(([&] { bob.start(p, "api"); bob.complete(p, "api"); })()) << "Proxy: and finish it";
    SecuredTividy rita(engine, "Rita");
    EXPECT_TRUE(rita.mayDecide(p, "api")) << "Proxy: the Reviewer may decide on api";
    EXPECT_NO_THROW(([&] { rita.reject(p, "api", "Needs tests"); })()) << "Proxy: and request changes";
    EXPECT_TRUE(engine.instance(p).item("api")->reason() == "Needs tests") << "Proxy: the reason reaches the item";
    EXPECT_NO_THROW(([&] { bob.start(p, "api"); })()) << "Proxy: Bob can pick rejected work back up";
    EXPECT_NO_THROW(([&] { bob.escalate(p, "api"); })()) << "Proxy: the assignee can escalate";
    EXPECT_THROW(([&] { fay.escalate(p, "migration"); })(), AccessDenied) << "Proxy: others cannot escalate someone else's work";
    EXPECT_NO_THROW(([&] { maya.assign(p, "api", "Ben"); })()) << "Proxy: a manager reassigns escalated work";
    EXPECT_NO_THROW(([&] { maya.addPerson("Eve", "Developer", "backend"); })()) << "Proxy: managers can add people";
    EXPECT_NO_THROW(([&] { maya.undo(); })()) << "Proxy: managers can undo";
}

TEST(Proxy, AutoApproval) {
    Tividy engine("Acme");
    DemoSeed::company(engine);
    ConcreteDefinitionBuilder b;
    b.reset("Tiny");
    b.setPolicy("SmallTeam");
    b.addStructuredStage(StageTemplate("only", "Only"));
    b.addStructuredItem("only", ItemTemplate("ship", "Ship it", "ReleaseManager", "platform", "Auto"));
    b.addStructuredItem("only", ItemTemplate("note", "Release note", "ReleaseManager", "platform", "Auto"));
    engine.loadDefinition("Tiny", b.build());

    std::string small = engine.startProject("Tiny", "", "SmallTeam");
    engine.autoAssign(small, "ship");
    engine.start(small, "ship");
    engine.complete(small, "ship");
    EXPECT_TRUE(engine.instance(small).item("ship")->status() == WorkItem::COMPLETED) << "Chain: SmallTeam auto-approves Auto items";

    std::string busy = engine.startProject("Tiny", "", "BusyTeam");
    engine.autoAssign(busy, "ship");
    engine.start(busy, "ship");
    engine.complete(busy, "ship");
    LeafWorkItem* ship = dynamic_cast<LeafWorkItem*>(engine.instance(busy).item("ship"));
    EXPECT_TRUE(ship->status() == WorkItem::AWAITING_APPROVAL && ship->pendingApprover() == "Manager") << "Chain: BusyTeam has no auto handler, so a Manager must approve";
    EXPECT_EQ(engine.approvalsFor("Nina").size(), 1u) << "Chain: every manager sees Manager approvals";
}

}

