-keepattributes Exceptions, InnerClasses
-keep public class org.babachain.wallet.common.** {
    public protected *;
}
-keep public interface org.babachain.wallet.common.** {*;}
-dontwarn java.lang.invoke.StringConcatFactory