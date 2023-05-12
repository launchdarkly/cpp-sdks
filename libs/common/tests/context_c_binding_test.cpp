#include <gtest/gtest.h>

#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/context_builder.h>

TEST(ContextCBindingTests, CanBuildBasicContext) {
    LDContextBuilder builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(builder, "user", "user-key");
    LDContext context = LDContextBuilder_Build(builder);

    EXPECT_EQ(std::string("user-key"),
              LDValue_GetString(LDContext_Get(context, "user", "key")));

    EXPECT_TRUE(LDContext_Valid(context));

    LDContext_Free(context);
}

TEST(ContextCBindingTests, CanAddAttributes) {
    LDContextBuilder builder = LDContextBuilder_New();

    LDContextBuilder_AddKind(builder, "user", "user-key");
    LDContextBuilder_Attributes_Set(builder, "user", "custom",
                                    LDValue_NewString("custom_value"));
    LDContextBuilder_Attributes_SetName(builder, "user", "Joe");
    LDContextBuilder_Attributes_SetAnonymous(builder, "user", true);

    LDContextBuilder_Attributes_SetPrivate(
        builder, "user", "custom_private",
        LDValue_NewString("this_is_private"));

    LDContextBuilder_Attributes_AddPrivateAttribute(builder, "user", "/email");

    LDContext context = LDContextBuilder_Build(builder);

    EXPECT_EQ(std::string("user-key"),
              LDValue_GetString(LDContext_Get(context, "user", "key")));

    EXPECT_EQ(std::string("custom_value"),
              LDValue_GetString(LDContext_Get(context, "user", "custom")));

    EXPECT_EQ(
        std::string("this_is_private"),
        LDValue_GetString(LDContext_Get(context, "user", "custom_private")));

    EXPECT_EQ(std::string("Joe"),
              LDValue_GetString(LDContext_Get(context, "user", "name")));

    EXPECT_TRUE(LDValue_GetBool(LDContext_Get(context, "user", "anonymous")));

    auto count = 0;

    LDContext_PrivateAttributesIter iter =
        LDContext_CreatePrivateAttributesIter(context, "user");

    while (!LDContext_PrivateAttributesIter_End(iter)) {
        if (count == 0) {
            EXPECT_EQ(std::string("custom_private"),
                      LDContext_PrivateAttributesIter_Value(iter));
        }
        if (count == 1) {
            EXPECT_EQ(std::string("/email"),
                      LDContext_PrivateAttributesIter_Value(iter));
        }
        LDContext_PrivateAttributesIter_Next(iter);
        count++;
    }

    LDContext_DestroyPrivateAttributesIter(iter);

    EXPECT_EQ(2, count);

    LDContext_Free(context);
}

TEST(ContextCBindingTests, CanMakeMultiKindContext) {
    LDContextBuilder builder = LDContextBuilder_New();

    LDContextBuilder_AddKind(builder, "user", "user-key");
    LDContextBuilder_Attributes_Set(builder, "user", "custom",
                                    LDValue_NewString("custom_value"));

    LDContextBuilder_AddKind(builder, "org", "org-key");
    LDContextBuilder_Attributes_SetName(builder, "org", "SDK");

    LDContextBuilder_Attributes_SetName(builder, "user", "Joe");
    LDContextBuilder_Attributes_SetAnonymous(builder, "user", true);

    LDContext context = LDContextBuilder_Build(builder);

    EXPECT_EQ(std::string("user-key"),
              LDValue_GetString(LDContext_Get(context, "user", "key")));

    EXPECT_EQ(std::string("custom_value"),
              LDValue_GetString(LDContext_Get(context, "user", "custom")));

    EXPECT_EQ(std::string("Joe"),
              LDValue_GetString(LDContext_Get(context, "user", "name")));

    EXPECT_TRUE(LDValue_GetBool(LDContext_Get(context, "user", "anonymous")));

    EXPECT_EQ(std::string("org-key"),
              LDValue_GetString(LDContext_Get(context, "org", "key")));

    EXPECT_EQ(std::string("SDK"),
              LDValue_GetString(LDContext_Get(context, "org", "name")));

    LDContext_Free(context);
}

TEST(ContextCBindingTests, CanCreateInvalidContext) {
    LDContextBuilder builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(builder, "#)(#$@*(#^@&*", "user-key");
    LDContext context = LDContextBuilder_Build(builder);

    EXPECT_FALSE(LDContext_Valid(context));

    EXPECT_EQ(std::string("#)(#$@*(#^@&*: \"Kind contained invalid characters. "
                          "A kind may contain ASCII letters or numbers, as "
                          "well as '.', '-', and '_'.\""),
              LDContext_Errors(context));

    LDContext_Free(context);
}
