<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                xmlns:xi="http://www.w3.org/2001/XInclude"
                xmlns:fox="http://xmlgraphics.apache.org/fop/extensions"
                version="1.0"
>

<!-- use prog 'create-book-covers.pl' -->

<xsl:include href="brlcad-gendata.xsl"/>

<!-- ==================================================================== -->
<xsl:template name="front.cover">

  <xsl:variable name='logosize'>40pt</xsl:variable>
  <xsl:variable name='brlcadsize'>40pt</xsl:variable><!-- started with 40pt -->

  <fo:page-sequence master-reference="front-cover">

    <!--
      force-page-count="no-force">
    -->

    <fo:flow flow-name="xsl-region-body">

      <?brl-cad insert-draft-overlay ?>

      <!-- BRL-CAD LOGO ====================================================== -->
      <!-- this is the BRL-CAD Logo; point size is for the 'BRL-CAD', the rest
      of the container has inherited sizes scaled as a proportion of that size
      so as to meet the company logo rules -->
      <!-- final position on the page should have the top at 0.35in -->
      <fo:block-container font-size="{$mtlogosize}"
         line-height='50.0%'
         text-align="right"
         font-family='Bembo'
         absolute-position='fixed'
         right='0.50in'
         top='0.40in'
         >
        <fo:block color='red'>
          BRL-CAD
        </fo:block>
        <fo:block font-size='36.0%' last-line-end-indent='{$mtlogosize} * -0.10'>
          <!-- reg mark looks like 0.4 size of normal 'n' -->
          International Corporation<fo:inline font-size='29.8%'
             baseline-shift='1.0%'>&#xAE;</fo:inline><!-- &lt;= R => C &#xA9; -->
        </fo:block>
      </fo:block-container>

      <!-- TOP RULE ================================ -->
      <fo:block-container  top="1in" absolute-position="absolute">
        <fo:block text-align='center'>
           <fo:leader leader-length="8.5in"
             leader-pattern="rule"
             alignment-baseline="middle"
             rule-thickness="2pt" color="red"/>
        </fo:block>
      </fo:block-container>

      <?brl-cad insert-value-logo-group ?>

      <!-- DOCUMENT TITLE ================================================== -->
      <?brl-cad insert-title ?>

      <!-- GENERATION AND REVISION DATE ==================================== -->
      <fo:block-container
         top="7.6in"
         text-align="center" absolute-position="absolute" font-family="LinLib"
         font-size='8'
        >

         <fo:block>
           VALUE <xsl:value-of select="$brlcad.vcs"/>
               Revision: <xsl:value-of select="$brlcad.revision"/>
         </fo:block>
         <fo:block>
           PDF Generation Date <xsl:value-of select="$brlcad.pdf.gendate"/>
         </fo:block>
      </fo:block-container>

      <!-- BOTTOM  RULE ================================ -->
      <fo:block-container top="10in" absolute-position="absolute">
        <fo:block text-align='center'>
           <fo:leader leader-length="8.5in"
             leader-pattern="rule"
             alignment-baseline="middle"
             rule-thickness="2pt" color="red"/>
        </fo:block>
      </fo:block-container>

      <!-- BOTTOM COPYRIGHT ================================ -->
      <fo:block-container absolute-position="absolute" top="10.25in" left="0.5in"
          right="0.5in" bottom="1in" text-align="center" font-family="serif">
        <fo:block>Approved for public release; distribution is unlimited.</fo:block>
      </fo:block-container>


    </fo:flow>
  </fo:page-sequence>
</xsl:template>



<!-- ==================================================================== -->
<xsl:template name="back.cover">

  <fo:page-sequence master-reference="back-cover"
    initial-page-number="auto-even"
    >

    <fo:flow flow-name="xsl-region-body">

      <!-- TOP RULE ================================ -->
      <fo:block-container  top="1in" absolute-position="absolute">
        <fo:block text-align='center'>
           <fo:leader leader-length="8.5in"
             leader-pattern="rule"
             alignment-baseline="middle"
             rule-thickness="2pt" color="red"/>
        </fo:block>
      </fo:block-container>

      <!-- BOTTOM  RULE ================================ -->
      <fo:block-container top="10in" absolute-position="absolute">
        <fo:block text-align='center'>
           <fo:leader leader-length="8.5in"
             leader-pattern="rule"
             alignment-baseline="middle"
             rule-thickness="2pt" color="red"/>
        </fo:block>
      </fo:block-container>

    </fo:flow>
  </fo:page-sequence>
</xsl:template>

<!-- ==================================================================== -->
  <!-- page sequence is defined as follows: -->
  <xsl:template name="user.pagemasters">

    <fo:simple-page-master master-name="front-cover" page-width="{$page.width}"
      page-height="{$page.height}" margin-top="0pt" margin-bottom="0pt"
      margin-left="0pt" margin-right="0pt">

      <fo:region-body margin="0in"/>

      <!--
      <fo:region-before extent="1in" background-color='lightblue'/>
      <fo:region-after extent="1in" background-color='lightblue'/>
      <fo:region-start extent="1in" background-color='lightgreen'/>
      <fo:region-end extent="1in" background-color='lightgreen'/>
      -->

    </fo:simple-page-master>

    <fo:simple-page-master master-name="back-cover" page-width="{$page.width}"
      page-height="{$page.height}" margin-top="0pt" margin-bottom="0pt"
      margin-left="0pt" margin-right="0pt">

      <fo:region-body margin="0in"/>

    </fo:simple-page-master>

  </xsl:template>



</xsl:stylesheet>
