from django.conf.urls.defaults import patterns, include
from django.contrib import admin
import views

admin.autodiscover()

urlpatterns = patterns( '',
    
    # Uncomment the admin/doc line below and add 'django.contrib.admindocs' 
    # to INSTALLED_APPS to enable admin documentation:
    ( r'^admin/doc/', include( 'django.contrib.admindocs.urls' ) ),

    # Uncomment the next line to enable the admin:
    ( r'^admin/', include( admin.site.urls ) ),
    
    # Top level links pages
    ( r'^$', views.default ),
    ( r'^about/$', views.default, { 'action' : 'about' } ),
    ( r'^references/$', views.default, { 'action' : 'references' }),
    ( r'^news/$', views.default, { 'action' : 'news' }),
 
)
