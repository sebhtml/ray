<?xml version="1.0" encoding="UTF-8"?>


<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
  <html>
<head>

<style>

body{
font-family: Arial;
}

.taxon{
border-style:solid; 
background: #66CCFF;

margin-top:50px;
margin-bottom:50px;
margin-right:50px;
margin-left:50px;

padding-top:30px;
padding-bottom:30px;
padding-right:30px;
padding-left:30px;
}

.name{
font-size: 34px;
}

.path{
background: #FF9966;
color:black;
border-style:solid; 
padding-top:20px;
padding-bottom:20px;
padding-right:20px;
padding-left:20px;
}

.observations{

padding-top:10px;
padding-bottom:10px;
padding-right:10px;
padding-left:10px;
}

.proportion{
font-size: 14px;

border-style:solid; 

margin: 5px;
padding-top:5px;
padding-bottom:5px;
padding-right:5px;
padding-left:5px;
}

</style>


<title>
Sample: <xsl:value-of select="root/sample"/>
</title>



</head>
  <body>
    <h1>Sample: <xsl:value-of select="root/sample"/></h1>
    <h2>Taxonomic profiling from BiologicalAbundances/_Taxonomy/Taxons.xml</h2>
	<h2>Produced by Ray technologies</h2>
<div>
Total assembled k-mer observations:
   <xsl:value-of select="root/totalAssembledKmerObservations"/>
</div>
<div>
Total colored assembled k-mer observations:
   <xsl:value-of select="root/totalColoredAssembledKmerObservations"/>
</div>

      <xsl:for-each select="root/entry">
<div class="taxon">
		<div class="name"><xsl:value-of select="taxon/name"/></div>

<div>
self

          	<div class="proportion"><xsl:value-of select="100* self/proportion"/>% <small>assembled</small>
<div class="observations">
          Assembled k-mer observations:
<xsl:value-of select="self/assembledKmerObservations"/> / <xsl:value-of select="/root/totalAssembledKmerObservations"/>
</div>
</div>



          	<div class="proportion"><xsl:value-of select="100* self/coloredProportion"/>% <small>colored</small>

<div class="observations">
          Assembled k-mer observations (over colored tribe):
<xsl:value-of select="self/assembledKmerObservations"/> / <xsl:value-of select="/root/totalColoredAssembledKmerObservations"/>
</div>
</div>
</div>

<div>
recursive

          	<div class="proportion"><xsl:value-of select="100* recursive/proportion"/>% <small>assembled</small>
<div class="observations">
          Assembled k-mer observations:
<xsl:value-of select="recursive/assembledKmerObservations"/> / <xsl:value-of select="/root/totalAssembledKmerObservations"/>
</div>
</div>



          	<div class="proportion"><xsl:value-of select="100* recursive/coloredProportion"/>% <small>colored</small>

<div class="observations">
          Assembled k-mer observations (over colored tribe):
<xsl:value-of select="recursive/assembledKmerObservations"/> / <xsl:value-of select="/root/totalColoredAssembledKmerObservations"/>
</div>
</div>

</div>


          <div class="path">

<ul>

      <xsl:for-each select="path/taxon">
<li>
<b><xsl:value-of select="rank"/>:</b>
	<xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
<xsl:value-of select="name"/> <br />
</li>
		
      </xsl:for-each>
</ul>

</div>

</div>
      </xsl:for-each>

  </body>
  </html>
</xsl:template>

</xsl:stylesheet> 
